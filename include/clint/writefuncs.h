#ifndef WRITEFUNCS_H
#define WRITEFUNCS_H

#include <stddef.h>
#include "common/config.h"

int writeFile(const dfs_config_t *config);
int write_block_execute(const dfs_config_t *config, const char *filename, int block_id, const void *payload, size_t payload_len);

#endif