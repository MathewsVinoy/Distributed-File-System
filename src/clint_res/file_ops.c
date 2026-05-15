#include "clint/file_ops.h"
#include "clint/lookup.h"
#include "common/logging.h"
#include "common/protocol.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

static int connect_to_metadata(const dfs_config_t *config) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        LOG_ERROR("Failed to create socket");
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(config->client.metadata_port);
    server_addr.sin_addr.s_addr = inet_addr(config->client.metadata_host);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        LOG_ERROR("Failed to connect to metadata server");
        close(sock);
        return -1;
    }

    return sock;
}

int dfs_delete_file(const dfs_config_t *config, const char *filename) {
    if (!config || !filename) {
        return -1;
    }

    // First, get the file map to find all blocks
    FileMap map;
    if (findlocation(config->metadata_file, filename, &map) != 0) {
        LOG_ERROR("File not found: %s", filename);
        return -1;
    }

    LOG_INFO("Deleting file: %s (%d blocks)", filename, map.total_blocks);

    // Send delete request to each data server for each block
    int errors = 0;
    for (int i = 0; i < map.total_blocks; i++) {
        for (size_t j = 0; j < MAX_DS; j++) {
            if (map.blocks[i].ports[j] == 0 || map.blocks[i].locations[j][0] == '\0') {
                continue;
            }

            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) {
                errors++;
                continue;
            }

            struct sockaddr_in ds_addr;
            memset(&ds_addr, 0, sizeof(ds_addr));
            ds_addr.sin_family = AF_INET;
            ds_addr.sin_port = htons(map.blocks[i].ports[j]);
            ds_addr.sin_addr.s_addr = inet_addr(map.blocks[i].locations[j]);

            if (connect(sock, (struct sockaddr *)&ds_addr, sizeof(ds_addr)) == 0) {
                char request[128];
                snprintf(request, sizeof(request), "DELETE BLOCK %d", map.blocks[i].blockid);
                send(sock, request, strlen(request), 0);
                
                LOG_DEBUG("Deleted block %d from %s:%d", 
                         map.blocks[i].blockid,
                         map.blocks[i].locations[j],
                         map.blocks[i].ports[j]);
            } else {
                errors++;
            }

            close(sock);
        }
    }

    // TODO: Remove metadata entry from metadata.txt
    // This would require implementing metadata modification
    
    if (errors > 0) {
        LOG_WARN("Deleted file with %d errors", errors);
    } else {
        LOG_INFO("File deleted successfully: %s", filename);
    }

    return (errors == 0) ? 0 : -1;
}

int dfs_rename_file(const dfs_config_t *config, const char *old_filename, const char *new_filename) {
    if (!config || !old_filename || !new_filename) {
        return -1;
    }

    // Check if old file exists
    FileMap map;
    if (findlocation(config->metadata_file, old_filename, &map) != 0) {
        LOG_ERROR("File not found: %s", old_filename);
        return -1;
    }

    // Check if new filename already exists
    FileMap check;
    if (findlocation(config->metadata_file, new_filename, &check) == 0) {
        LOG_ERROR("Destination file already exists: %s", new_filename);
        return -1;
    }

    // TODO: Implement metadata update
    // This would require:
    // 1. Read entire metadata file
    // 2. Find all entries for old_filename
    // 3. Replace with new_filename
    // 4. Write back to metadata file
    
    LOG_WARN("File rename not fully implemented - metadata update required");
    return -1;
}

int dfs_get_file_stats(const dfs_config_t *config, const char *filename, dfs_file_stats_t *stats) {
    if (!config || !filename || !stats) {
        return -1;
    }

    FileMap map;
    if (findlocation(config->metadata_file, filename, &map) != 0) {
        return -1;
    }

    strncpy(stats->filename, filename, MAX_FILENAME_LEN - 1);
    stats->filename[MAX_FILENAME_LEN - 1] = '\0';
    stats->total_blocks = map.total_blocks;
    stats->total_size = (size_t)map.total_blocks * config->block_size;
    
    // Count replicas for first block
    stats->replica_count = 0;
    if (map.total_blocks > 0) {
        for (size_t i = 0; i < MAX_DS; i++) {
            if (map.blocks[0].ports[i] != 0) {
                stats->replica_count++;
            }
        }
    }

    return 0;
}
