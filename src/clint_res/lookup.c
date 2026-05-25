#include "clint/lookup.h"
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "common/logging.h"

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

static int request_blockinfo(int meta_socket, const char *filename, int block_id, blockinfo *out_location) {
    char comment[128];
    snprintf(comment, sizeof(comment), "LOOKUP %.*s BLOCK %d", MAX_FILENAME_LEN - 1, filename, block_id);
    if (send(meta_socket, comment, strlen(comment), 0) < 0) {
        perror("Failed to send lookup request");
        return -1;
    }

    blockinfo tmp;
    ssize_t bytes = recv(meta_socket, &tmp, sizeof(tmp), 0);
    if (bytes != sizeof(tmp) || tmp.blockid < 0) {
        return -1;
    }
    if (out_location) {
        memcpy(out_location, &tmp, sizeof(blockinfo));
    }
    return 0;
}

static ssize_t fetch_replica_sample(const blockinfo *location, char *buffer, size_t buffer_len) {
    if (!location || !buffer || buffer_len == 0) {
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;

    for (size_t i = 0; i < MAX_DS; i++) {
        if (location->ports[i] <= 0 || location->locations[i][0] == '\0') {
            continue;
        }

        int ds_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (ds_socket == -1) {
            perror("Socket creation failed");
            continue;
        }

        server_addr.sin_port = htons(location->ports[i]);
        server_addr.sin_addr.s_addr = inet_addr(location->locations[i]);

        if (connect(ds_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("Data server connection failed");
            close(ds_socket);
            continue;
        }

        char request[64];
        snprintf(request, sizeof(request), "GET BLOCK %d", location->blockid);
        if (send(ds_socket, request, strlen(request), 0) < 0) {
            close(ds_socket);
            continue;
        }

        memset(buffer, 0, buffer_len);
        ssize_t received = recv(ds_socket, buffer, buffer_len - 1, 0);
        close(ds_socket);
        if (received > 0) {
            buffer[received] = '\0';
            return received;
        }

        LOG_WARN("Failed to receive data from %s:%d - %s",
                 location->locations[i],
                 location->ports[i],
                 strerror(errno));
    }

    return -1;
}

int lookup_block_execute(const dfs_config_t *config, const char *filename, int block_id, blockinfo *out_location) {
    if (!config || !filename || !out_location) {
        return -1;
    }

    int meta_socket = open_metadata_socket(config);
    if (meta_socket < 0) {
        return -1;
    }

    int rc = request_blockinfo(meta_socket, filename, block_id, out_location);
    close(meta_socket);
    if (rc != 0) {
        LOG_WARN("Metadata lookup failed for %s block %d", filename, block_id);
    }
    return rc;
}

int lookupBlock(const dfs_config_t *config) {
    if (!config) {
        return -1;
    }

    char filename[MAX_FILENAME_LEN];
    int block_id;
    printf("Filename: ");
    scanf("%49s", filename);
    printf("Block id: ");
    scanf("%d", &block_id);
    filename[MAX_FILENAME_LEN - 1] = '\0';

    blockinfo location;
    if (lookup_block_execute(config, filename, block_id, &location) != 0) {
        return -1;
    }

    char buffer[1024];
    ssize_t bytes = fetch_replica_sample(&location, buffer, sizeof(buffer));
    if (bytes > 0) {
        printf("Replica %d preview (%zd bytes): %s\n", location.blockid, bytes, buffer);
        return 0;
    }

    LOG_ERROR("All replicas failed for %s block %d", filename, block_id);
    return -1;
}

