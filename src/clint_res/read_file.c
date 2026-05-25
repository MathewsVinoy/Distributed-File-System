#include "clint/read_file.h"
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "clint/connction_cash.h"
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

static int fetch_file_map(const dfs_config_t *config, const char *filename, FileMap *out_map) {
    int meta_socket = open_metadata_socket(config);
    if (meta_socket < 0) {
        return -1;
    }

    char comment[160];
    snprintf(comment, sizeof(comment), "GET_FILE_MAP %.*s", MAX_FILENAME_LEN - 1, filename);
    if (send(meta_socket, comment, strlen(comment), 0) < 0) {
        close(meta_socket);
        return -1;
    }

    FileMap tmp;
    ssize_t bytes = recv(meta_socket, &tmp, sizeof(tmp), 0);
    close(meta_socket);
    if (bytes != sizeof(tmp) || tmp.total_blocks <= 0) {
        LOG_WARN("Invalid file map for %s", filename);
        return -1;
    }

    if (out_map) {
        memcpy(out_map, &tmp, sizeof(FileMap));
    }
    return 0;
}

int read_file_execute(const dfs_config_t *config, const char *filename, const char *output_path) {
    if (!config || !filename) {
        return -1;
    }

    const char *destination = output_path && output_path[0] ? output_path : config->client.output_file;
    FileMap datamap;
    if (fetch_file_map(config, filename, &datamap) != 0) {
        return -1;
    }

    FILE *fp = fopen(destination, "wb");
    if (!fp) {
        perror("Failed to open output file");
        return -1;
    }

    for (int i = 0; i < datamap.total_blocks; i++) {
        int success = 0;
        for (size_t j = 0; j < MAX_DS; j++) {
            if (datamap.blocks[i].ports[j] == 0 || datamap.blocks[i].locations[j][0] == '\0') {
                continue;
            }

            int ds_socket = get_connection(datamap.blocks[i].locations[j], datamap.blocks[i].ports[j]);
            if (ds_socket < 0) {
                continue;
            }

            char request[64];
            snprintf(request, sizeof(request), "GET BLOCK %d", datamap.blocks[i].blockid);
            send(ds_socket, request, strlen(request), 0);

            char buffer[1024];
            memset(buffer, 0, sizeof(buffer));
            ssize_t n = recv(ds_socket, buffer, sizeof(buffer), 0);

            if (n > 0) {
                fwrite(buffer, 1, (size_t)n, fp);
                release_connection(ds_socket);
                success = 1;
                break;
            }

            LOG_WARN("Failed to receive block %d from %s:%d - %s",
                     datamap.blocks[i].blockid,
                     datamap.blocks[i].locations[j],
                     datamap.blocks[i].ports[j],
                     strerror(errno));
            release_connection(ds_socket);
        }

        if (!success) {
            LOG_ERROR("Failed to read block %d", datamap.blocks[i].blockid);
            fclose(fp);
            close_all_connections();
            return -1;
        }
    }

    fclose(fp);
    close_all_connections();
    LOG_INFO("File %s written to %s", filename, destination);
    return 0;
}

int readFile(const dfs_config_t *config) {
    if (!config) {
        return -1;
    }

    char filename[MAX_FILENAME_LEN];
    printf("Filename: ");
    scanf("%49s", filename);
    filename[MAX_FILENAME_LEN - 1] = '\0';

    int rc = read_file_execute(config, filename, config->client.output_file);
    if (rc == 0) {
        printf("File written to %s\n", config->client.output_file);
    }
    return rc;
}

