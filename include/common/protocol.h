#ifndef DFS_PROTOCOL_H
#define DFS_PROTOCOL_H

#include "datastruct.h"

typedef enum {
    METADATA_CMD_INVALID = 0,
    METADATA_CMD_LOOKUP_BLOCK,
    METADATA_CMD_GET_FILE_MAP,
    METADATA_CMD_WRITE_BLOCK
} metadata_command_t;

typedef struct {
    metadata_command_t cmd;
    char filename[MAX_FILENAME_LEN];
    int block_id;
} metadata_request_t;

int parse_metadata_request(const char *buffer, metadata_request_t *out_request);
const char *metadata_command_to_string(metadata_command_t cmd);

#endif
