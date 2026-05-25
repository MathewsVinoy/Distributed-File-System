#include "clint/writefuncs.h"
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "common/logging.h"
#include "datastruct.h"

static int open_metadata_socket(const dfs_config_t *config) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config->client.metadata_port);
    server_addr.sin_addr.s_addr = inet_addr(config->client.metadata_host);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Metadata server connection failed");
        close(sock);
        return -1;
    }
    return sock;
}

static int resolve_replica_set(const dfs_config_t *config, const char *filename, int block_id, blockinfo *location) {
    int meta_socket = open_metadata_socket(config);
    if (meta_socket < 0) {
        return -1;
    }

    char comment[160];
    snprintf(comment, sizeof(comment), "WRITE_BLOCK %.*s BLOCK %d", MAX_FILENAME_LEN - 1, filename, block_id);
    if (send(meta_socket, comment, strlen(comment), 0) < 0) {
        close(meta_socket);
        return -1;
    }

    blockinfo tmp;
    ssize_t recv_bytes = recv(meta_socket, &tmp, sizeof(tmp), 0);
    close(meta_socket);
    if (recv_bytes != sizeof(tmp) || tmp.blockid < 0) {
        LOG_ERROR("Failed to resolve replica set for %s block %d", filename, block_id);
        return -1;
    }

    if (location) {
        memcpy(location, &tmp, sizeof(blockinfo));
    }
    return 0;
}

static int push_payload_to_replica(const blockinfo *location, size_t replica_idx, int block_id, const void *payload, size_t payload_len) {
    if (!location || !payload || payload_len == 0) {
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(location->ports[replica_idx]);
    server_addr.sin_addr.s_addr = inet_addr(location->locations[replica_idx]);

    int ds_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (ds_socket == -1) {
        perror("Socket creation failed");
        return -1;
    }

    if (connect(ds_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Data server connection failed");
        close(ds_socket);
        return -1;
    }

    char request[64];
    snprintf(request, sizeof(request), "PUT BLOCK %d", block_id);
    if (send(ds_socket, request, strlen(request), 0) < 0) {
        close(ds_socket);
        return -1;
    }

    char ack[32];
    memset(ack, 0, sizeof(ack));
    ssize_t ack_bytes = recv(ds_socket, ack, sizeof(ack) - 1, 0);
    if (ack_bytes <= 0 || strncmp(ack, "ok", 2) != 0) {
        LOG_WARN("Replica %s:%d rejected PUT", location->locations[replica_idx], location->ports[replica_idx]);
        close(ds_socket);
        return -1;
    }

    if (send(ds_socket, payload, payload_len, 0) < 0) {
        close(ds_socket);
        return -1;
    }

    memset(ack, 0, sizeof(ack));
    ack_bytes = recv(ds_socket, ack, sizeof(ack) - 1, 0);
    close(ds_socket);

    if (ack_bytes > 0 && strncmp(ack, "ok", 2) == 0) {
        LOG_INFO("Replica %s:%d accepted data (%zu bytes)",
                 location->locations[replica_idx],
                 location->ports[replica_idx],
                 payload_len);
        return 0;
    }

    LOG_WARN("Replica %s:%d failed to persist data - %s",
             location->locations[replica_idx],
             location->ports[replica_idx],
             strerror(errno));
    return -1;
}

int write_block_execute(const dfs_config_t *config, const char *filename, int block_id, const void *payload, size_t payload_len) {
    if (!config || !filename || !payload || payload_len == 0) {
        return -1;
    }

    blockinfo location;
    if (resolve_replica_set(config, filename, block_id, &location) != 0) {
        return -1;
    }

    int replica_success = 0;
    for (size_t i = 0; i < MAX_DS; i++) {
        if (location.ports[i] == 0 || location.locations[i][0] == '\0') {
            continue;
        }
        if (push_payload_to_replica(&location, i, block_id, payload, payload_len) == 0) {
            replica_success++;
        }
    }

    if (replica_success == 0) {
        LOG_ERROR("No replicas accepted payload for block %d", block_id);
        return -1;
    }
    return 0;
}

int writeFile(const dfs_config_t *config) {
    if (!config) {
        return -1;
    }

    char filename[MAX_FILENAME_LEN];
    int block_id;
    char local_path[DFS_PATH_LEN];

    printf("Filename: ");
    scanf("%49s", filename);
    printf("Block id: ");
    scanf("%d", &block_id);
    filename[MAX_FILENAME_LEN - 1] = '\0';
    printf("Local file to upload: ");
    scanf("%255s", local_path);

    FILE *input = fopen(local_path, "rb");
    if (!input) {
        perror("Failed to open local file");
        return -1;
    }

    char *buffer = malloc(config->block_size);
    if (!buffer) {
        fclose(input);
        return -1;
    }

    size_t bytes_to_send = fread(buffer, 1, config->block_size, input);
    fclose(input);
    if (bytes_to_send == 0) {
        LOG_ERROR("No data read from %s", local_path);
        free(buffer);
        return -1;
    }

    int rc = write_block_execute(config, filename, block_id, buffer, bytes_to_send);
    free(buffer);
    return rc;
}

