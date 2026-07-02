#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "common/config.h"
#include "common/logging.h"
#include "dataserver/heatbeat.h"

typedef struct {
    int server_fd;
    dfs_config_t config;
    int port;
} dataserver_context_t;

static int bind_server_socket(const char *addr, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        LOG_ERROR("Socket creation failed: %s", strerror(errno));
        return -1;
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(addr);
    server_addr.sin_port = htons(port);

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

static int build_temp_path(char *dest, size_t len, const dataserver_context_t *ctx, int block_id) {
    size_t dir_len = strnlen(ctx->config.data.log_dir, len - 1);
    if (dir_len >= len) {
        LOG_ERROR("Log directory path too long: %s", ctx->config.data.log_dir);
        return -1;
    }
    memcpy(dest, ctx->config.data.log_dir, dir_len);
    if (dir_len + 1 >= len) {
        return -1;
    }
    dest[dir_len] = '\0';
    size_t remaining = len - dir_len;
    int written = snprintf(dest + dir_len, remaining, "/temp%d_block%d.txt", ctx->port, block_id);
    if (written < 0 || (size_t)written >= remaining) {
        LOG_ERROR("Temp path exceeds buffer for block %d", block_id);
        return -1;
    }
    return 0;
}

static void ensure_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        mkdir(path, 0755);
    }
}

static void handle_get_request(int client_sock, const dataserver_context_t *ctx, int block_id, char *buffer) {
    if (block_id < 0) {
        const char *msg = "ERROR: Missing block id";
        send(client_sock, msg, strlen(msg), 0);
        return;
    }

    FILE *fp = fopen(ctx->config.data.data_file, "rb");
    if (!fp) {
        LOG_ERROR("Failed to open data file %s", ctx->config.data.data_file);
        const char *msg = "ERROR: File open failed";
        send(client_sock, msg, strlen(msg), 0);
        return;
    }

    long offset = (long)block_id * (long)ctx->config.block_size;
    if (fseek(fp, offset, SEEK_SET) != 0) {
        LOG_ERROR("fseek failed for block %d", block_id);
    }

    size_t read_bytes = fread(buffer, 1, ctx->config.block_size, fp);
    if (read_bytes == 0 && ferror(fp)) {
        LOG_ERROR("fread failed for block %d", block_id);
    } else {
        send(client_sock, buffer, read_bytes, 0);
    }
    fclose(fp);
}

static void handle_put_request(int client_sock, const dataserver_context_t *ctx, int block_id, char *buffer) {
    if (block_id < 0) {
        const char *msg = "ERROR: Missing block id";
        send(client_sock, msg, strlen(msg), 0);
        return;
    }

    ensure_directory(ctx->config.data.log_dir);

    char file_path[DFS_PATH_LEN];
    if (build_temp_path(file_path, sizeof(file_path), ctx, block_id) != 0) {
        const char *msg = "ERROR: Path build failed";
        send(client_sock, msg, strlen(msg), 0);
        return;
    }

    const char *ok_msg = "ok";
    send(client_sock, ok_msg, strlen(ok_msg), 0);

    FILE *fp = fopen(file_path, "wb");
    if (!fp) {
        LOG_ERROR("Failed to open temp log %s", file_path);
        const char *msg = "ERROR: File open failed";
        send(client_sock, msg, strlen(msg), 0);
        return;
    }

    ssize_t received = recv(client_sock, buffer, ctx->config.block_size, 0);
    if (received <= 0) {
        LOG_ERROR("Failed to receive block data for %d", block_id);
        const char *msg = "ERROR: Data receive failed";
        send(client_sock, msg, strlen(msg), 0);
        fclose(fp);
        return;
    }

    if (fwrite(buffer, 1, (size_t)received, fp) != (size_t)received) {
        LOG_ERROR("Failed to persist temp data for block %d", block_id);
        const char *msg = "ERROR: Write failed";
        send(client_sock, msg, strlen(msg), 0);
    } else {
        /* Sync to disk for durability */
        fflush(fp);
        int fd = fileno(fp);
        if (fd >= 0) {
            fsync(fd);
        }
        send(client_sock, ok_msg, strlen(ok_msg), 0);
    }
    fclose(fp);
}

static void apply_commit(const dataserver_context_t *ctx, int block_id, char *buffer) {
    char file_path[DFS_PATH_LEN];
    if (build_temp_path(file_path, sizeof(file_path), ctx, block_id) != 0) {
        return;
    }

    FILE *fp_temp = fopen(file_path, "rb");
    if (!fp_temp) {
        LOG_ERROR("Temp file missing for block %d", block_id);
        return;
    }
    size_t read_bytes = fread(buffer, 1, ctx->config.block_size, fp_temp);
    fclose(fp_temp);

    FILE *fp_data = fopen(ctx->config.data.data_file, "r+b");
    if (!fp_data) {
        fp_data = fopen(ctx->config.data.data_file, "wb");
    }
    if (!fp_data) {
        LOG_ERROR("Failed to open data file for commit");
        return;
    }
    long offset = (long)block_id * (long)ctx->config.block_size;
    if (fseek(fp_data, offset, SEEK_SET) == 0) {
        fwrite(buffer, 1, read_bytes, fp_data);
        /* Sync to disk for durability */
        fflush(fp_data);
        int fd = fileno(fp_data);
        if (fd >= 0) {
            fsync(fd);
        }
    } else {
        LOG_ERROR("fseek failed during commit for block %d", block_id);
    }
    fclose(fp_data);
    remove(file_path);
}

static void apply_abort(const dataserver_context_t *ctx, int block_id) {
    char file_path[DFS_PATH_LEN];
    if (build_temp_path(file_path, sizeof(file_path), ctx, block_id) != 0) {
        return;
    }
    remove(file_path);
}

static void process_connection(int client_sock, dataserver_context_t *ctx, char *buffer) {
    while (1) {
        char request[128];
        ssize_t req_len = recv(client_sock, request, sizeof(request) - 1, 0);
        if (req_len <= 0) {
            break;
        }
        request[req_len] = '\0';
        LOG_DEBUG("request: %s", request);

        char command[32] = {0};
        char keyword[16] = {0};
        int block_id = -1;
        int parsed = sscanf(request, "%31s %15s %d", command, keyword, &block_id);

        if (parsed < 2 || strcmp(keyword, "BLOCK") != 0) {
            const char *msg = "ERROR: Invalid request format";
            send(client_sock, msg, strlen(msg), 0);
            break;
        }

        if (strcmp(command, "GET") == 0) {
            handle_get_request(client_sock, ctx, block_id, buffer);
            break;
        } else if (strcmp(command, "PUT") == 0) {
            handle_put_request(client_sock, ctx, block_id, buffer);
            break;
        } else if (strcmp(command, "PREPARE_WRITE") == 0) {
            const char *ready_msg = "ok";
            send(client_sock, ready_msg, strlen(ready_msg), 0);
            continue;
        } else if (strcmp(command, "GLOBAL_COMMIT") == 0) {
            apply_commit(ctx, block_id, buffer);
            const char *msg = "COMMIT_OK";
            send(client_sock, msg, strlen(msg), 0);
            break;
        } else if (strcmp(command, "GLOBAL_ABORT") == 0) {
            apply_abort(ctx, block_id);
            const char *msg = "ABORTED";
            send(client_sock, msg, strlen(msg), 0);
            break;
        } else {
            const char *msg = "ERROR: Unknown command";
            send(client_sock, msg, strlen(msg), 0);
            break;
        }
    }
    close(client_sock);
}

int main(int argc, char *argv[]){
    const char *config_path = "config/dfs.conf";
    int port = 0;
    if (argc >= 2) {
        port = atoi(argv[1]);
    }
    if (argc >= 3) {
        config_path = argv[2];
    }

    dfs_config_t config;
    if (dfs_config_load(config_path, &config) != 0) {
        fprintf(stderr, "Failed to load config: %s\n", dfs_config_get_error());
        return EXIT_FAILURE;
    }

    if (port <= 0) {
        port = config.data.port;
    }
    config.data.port = port;

    log_init(stderr);
    log_set_level(LOG_LEVEL_INFO);

    dataserver_context_t ctx = {
        .server_fd = -1,
        .config = config,
        .port = port
    };

    ctx.server_fd = bind_server_socket(ctx.config.data.bind_addr, port);
    if (ctx.server_fd < 0) {
        return EXIT_FAILURE;
    }

    pthread_t thread_id;
    dataserver_heartbeat_args_t *hb_args = malloc(sizeof(dataserver_heartbeat_args_t));
    if (!hb_args) {
        LOG_ERROR("Failed to allocate heartbeat args");
        return EXIT_FAILURE;
    }
    strncpy(hb_args->host, ctx.config.data.heartbeat_host, DFS_ADDR_LEN - 1);
    hb_args->host[DFS_ADDR_LEN - 1] = '\0';
    hb_args->port = ctx.config.data.heartbeat_port;
    hb_args->data_server_port = port;
    if (pthread_create(&thread_id, NULL, heartbeat, hb_args) != 0) {
        LOG_ERROR("Failed to start heartbeat thread");
        return EXIT_FAILURE;
    }

    char *buffer = malloc(ctx.config.block_size);
    if (!buffer) {
        LOG_ERROR("Failed to allocate block buffer");
        close(ctx.server_fd);
        return EXIT_FAILURE;
    }

    LOG_INFO("Data server listening on %s:%d", ctx.config.data.bind_addr, port);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client_sock = accept(ctx.server_fd, (struct sockaddr *)&client_addr, &len);
        if (client_sock < 0) {
            LOG_WARN("Accept failed: %s", strerror(errno));
            continue;
        }
        process_connection(client_sock, &ctx, buffer);
    }

    free(buffer);
    close(ctx.server_fd);
    pthread_join(thread_id, NULL);
    log_shutdown();
    return 0;
}


