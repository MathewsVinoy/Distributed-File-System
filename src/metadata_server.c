#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <openssl/ssl.h>
#include "common/config.h"
#include "common/logging.h"
#include "common/protocol.h"
#include "common/threadpool.h"
#include "common/tls.h"
#include "load_metadata.h"
#include "datastruct.h"
#include "metadata/getlocation.h"
#include "metadata/writeblock.h"
#include "metadata/handleheartbeat.h"

typedef struct {
    int client_socket;
    const dfs_config_t *config;
    SSL *ssl;
    SSL_CTX *dataserver_ctx;
} metadata_task_t;

static ssize_t connection_read(int fd, SSL *ssl, void *buf, size_t len) {
    return tls_read_data(ssl, fd, buf, len);
}

static ssize_t connection_write(int fd, SSL *ssl, const void *buf, size_t len) {
    return tls_write_data(ssl, fd, buf, len);
}

static int create_listen_socket(const char *addr, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        LOG_ERROR("Socket creation failed on %s:%d - %s", addr, port, strerror(errno));
        return -1;
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(addr);

    if (bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        LOG_ERROR("Bind failed on %s:%d - %s", addr, port, strerror(errno));
        close(fd);
        return -1;
    }

    if (listen(fd, 16) < 0) {
        LOG_ERROR("Listen failed on %s:%d - %s", addr, port, strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}

static void send_blockinfo(int sock, SSL *ssl, const blockinfo *info) {
    blockinfo reply;
    if (info) {
        memcpy(&reply, info, sizeof(reply));
    } else {
        memset(&reply, 0, sizeof(reply));
        reply.blockid = -1;
    }
    connection_write(sock, ssl, &reply, sizeof(reply));
}

static void send_filemap(int sock, SSL *ssl, const FileMap *map) {
    FileMap tmp;
    if (map) {
        memcpy(&tmp, map, sizeof(tmp));
    } else {
        memset(&tmp, 0, sizeof(tmp));
    }
    connection_write(sock, ssl, &tmp, sizeof(tmp));
}

static void handle_client_request(void *arg) {
    metadata_task_t *task = (metadata_task_t *)arg;
    int client_sock = task->client_socket;
    const dfs_config_t *config = task->config;
    SSL *ssl = task->ssl;
    SSL_CTX *dataserver_ctx = task->dataserver_ctx;
    free(task);

    char buffer[1024];
    ssize_t bytes = connection_read(client_sock, ssl, buffer, sizeof(buffer) - 1);
    if (bytes <= 0) {
        LOG_WARN("Failed to read request from client");
        tls_close_connection(ssl);
        close(client_sock);
        return;
    }
    buffer[bytes] = '\0';

    metadata_request_t request;
    if (parse_metadata_request(buffer, &request) != 0) {
        LOG_WARN("Invalid request received: %s", buffer);
        const char *msg = "ERROR:INVALID_REQUEST";
        connection_write(client_sock, ssl, msg, strlen(msg));
        tls_close_connection(ssl);
        close(client_sock);
        return;
    }

    LOG_INFO("Handling %s for %s block %d",
             metadata_command_to_string(request.cmd),
             request.filename,
             request.block_id);

    if (request.cmd == METADATA_CMD_GET_FILE_MAP) {
        FileMap map;
        if (findlocation(config->metadata_file, request.filename, &map) == 0) {
            send_filemap(client_sock, ssl, &map);
        } else {
            LOG_WARN("File map not found for %s", request.filename);
            send_filemap(client_sock, ssl, NULL);
        }
        tls_close_connection(ssl);
        close(client_sock);
        return;
    }

    blockinfo info;
    if (get_location(config->metadata_file, request.filename, request.block_id, &info) == 0) {
        send_blockinfo(client_sock, ssl, &info);
    } else {
        LOG_WARN("Block %d not found for %s", request.block_id, request.filename);
        send_blockinfo(client_sock, ssl, NULL);
    }

    if (request.cmd == METADATA_CMD_WRITE_BLOCK) {
        FileMap datamap;
        if (findlocation(config->metadata_file, request.filename, &datamap) == 0) {
            write_session_t session;
            if (prepare_write_block(&datamap, request.block_id, &session, config, dataserver_ctx) == 0) {
                status_listen(&session, request.block_id);
            }
        }
    }

    tls_close_connection(ssl);
    close(client_sock);
}

int main(int argc, char *argv[]){
    const char *config_path = (argc > 1) ? argv[1] : "config/dfs.conf";
    dfs_config_t config;
    if (dfs_config_load(config_path, &config) != 0) {
        fprintf(stderr, "Failed to load config: %s\n", dfs_config_get_error());
        return EXIT_FAILURE;
    }

    log_init(stderr);
    log_set_level(LOG_LEVEL_INFO);
    tls_library_init();

    SSL_CTX *server_ctx = tls_create_server_ctx(&config.metadata.tls);
    SSL_CTX *dataserver_ctx = tls_create_client_ctx(&config.data.tls);
    if (config.metadata.tls.enabled && !server_ctx) {
        LOG_ERROR("Failed to initialize metadata TLS server context");
        if (server_ctx) SSL_CTX_free(server_ctx);
        return EXIT_FAILURE;
    }
    if (config.data.tls.enabled && !dataserver_ctx) {
        LOG_ERROR("Failed to initialize metadata->dataserver TLS context");
        if (server_ctx) SSL_CTX_free(server_ctx);
        return EXIT_FAILURE;
    }

    int server_fd = create_listen_socket(config.metadata.listen_addr, config.metadata.listen_port);
    if (server_fd < 0) {
        return EXIT_FAILURE;
    }

    int heartbeat_fd = create_listen_socket(config.metadata.listen_addr, config.metadata.heartbeat_port);
    if (heartbeat_fd < 0) {
        close(server_fd);
        return EXIT_FAILURE;
    }

    pthread_t hb_thread;
    heartbeat_thread_args_t *hb_args = malloc(sizeof(heartbeat_thread_args_t));
    if (!hb_args) {
        LOG_ERROR("Failed to allocate heartbeat arguments");
        close(server_fd);
        close(heartbeat_fd);
        return EXIT_FAILURE;
    }
    hb_args->server_fd = heartbeat_fd;
    hb_args->client_addr = malloc(sizeof(struct sockaddr_in));
    hb_args->addr_len = malloc(sizeof(socklen_t));
    hb_args->last_seen_path = config.last_seen_file;
    hb_args->tls_ctx = server_ctx;
    hb_args->tls_enabled = config.metadata.tls.enabled;
    if (!hb_args->client_addr || !hb_args->addr_len) {
        LOG_ERROR("Failed to allocate heartbeat address structures");
        free(hb_args->client_addr);
        free(hb_args->addr_len);
        free(hb_args);
        close(server_fd);
        close(heartbeat_fd);
        return EXIT_FAILURE;
    }
    *hb_args->addr_len = sizeof(struct sockaddr_in);

    if (pthread_create(&hb_thread, NULL, handleHeartBeat, hb_args) != 0) {
        LOG_ERROR("Failed to start heartbeat handler");
        return EXIT_FAILURE;
    }
    pthread_detach(hb_thread);

    threadpool_t pool;
    if (threadpool_init(&pool, 4) != 0) {
        LOG_ERROR("Failed to initialize worker pool");
        close(server_fd);
        close(heartbeat_fd);
        return EXIT_FAILURE;
    }

    LOG_INFO("Metadata server listening on %s:%d", config.metadata.listen_addr, config.metadata.listen_port);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client_sock = accept(server_fd, (struct sockaddr *)&client_addr, &len);
        if (client_sock < 0) {
            LOG_WARN("Accept failed: %s", strerror(errno));
            continue;
        }

        metadata_task_t *task = malloc(sizeof(metadata_task_t));
        if (!task) {
            LOG_ERROR("Failed to allocate client task");
            close(client_sock);
            continue;
        }
        task->client_socket = client_sock;
        task->config = &config;
        task->ssl = NULL;
        task->dataserver_ctx = dataserver_ctx;

        if (server_ctx && config.metadata.tls.enabled) {
            SSL *ssl = tls_accept_connection(server_ctx, client_sock);
            if (!ssl) {
                close(client_sock);
                free(task);
                continue;
            }
            task->ssl = ssl;
        }

        if (threadpool_submit(&pool, handle_client_request, task) != 0) {
            LOG_ERROR("Failed to submit client task");
            if (task->ssl) {
                tls_close_connection(task->ssl);
            }
            close(client_sock);
            free(task);
        }
    }

    threadpool_shutdown(&pool);
    close(server_fd);
    close(heartbeat_fd);
    if (server_ctx) {
        SSL_CTX_free(server_ctx);
    }
    if (dataserver_ctx) {
        SSL_CTX_free(dataserver_ctx);
    }
    tls_library_cleanup();
    log_shutdown();
    return 0;
}

