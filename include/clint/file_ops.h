#ifndef DFS_FILE_OPS_H
#define DFS_FILE_OPS_H

#include "datastruct.h"
#include "common/config.h"

/**
 * Delete a file from the distributed file system
 * Returns 0 on success, -1 on failure
 */
int dfs_delete_file(const dfs_config_t *config, const char *filename);

/**
 * Rename/move a file in the distributed file system
 * Returns 0 on success, -1 on failure
 */
int dfs_rename_file(const dfs_config_t *config, const char *old_filename, const char *new_filename);

/**
 * Get file statistics
 * Returns 0 on success, -1 on failure
 */
typedef struct {
    char filename[MAX_FILENAME_LEN];
    int total_blocks;
    size_t total_size;
    int replica_count;
} dfs_file_stats_t;

int dfs_get_file_stats(const dfs_config_t *config, const char *filename, dfs_file_stats_t *stats);

#endif /* DFS_FILE_OPS_H */
