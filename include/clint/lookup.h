#ifndef LOOKUP_H
#define LOOKUP_H

#include "common/config.h"
#include "datastruct.h"

int lookupBlock(const dfs_config_t *config);
int lookup_block_execute(const dfs_config_t *config, const char *filename, int block_id, blockinfo *out_location);

#endif
