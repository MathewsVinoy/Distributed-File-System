#ifndef READ_FILE_H
#define READ_FILE_H

#include "common/config.h"

int readFile(const dfs_config_t *config);
int read_file_execute(const dfs_config_t *config, const char *filename, const char *output_path);

#endif