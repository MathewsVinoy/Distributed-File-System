#include "common/protocol.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void sanitize(char *dest, const char *src, size_t max_len) {
    if (!dest || !src) {
        return;
    }
    strncpy(dest, src, max_len - 1);
    dest[max_len - 1] = '\0';
}

const char *metadata_command_to_string(metadata_command_t cmd) {
    switch (cmd) {
        case METADATA_CMD_LOOKUP_BLOCK: return "LOOKUP";
        case METADATA_CMD_GET_FILE_MAP: return "GET_FILE_MAP";
        case METADATA_CMD_WRITE_BLOCK:  return "WRITE_BLOCK";
        default: return "INVALID";
    }
}

int parse_metadata_request(const char *buffer, metadata_request_t *out_request) {
    if (!buffer || !out_request) {
        return -1;
    }

    out_request->cmd = METADATA_CMD_INVALID;
    out_request->filename[0] = '\0';
    out_request->block_id = -1;

    char cmd[32];
    char filename[MAX_FILENAME_LEN];
    char keyword[32];
    int block_id = -1;

    int parsed = sscanf(buffer, "%31s %49s %31s %d", cmd, filename, keyword, &block_id);
    if (parsed < 2) {
        return -1;
    }

    if (strcmp(cmd, "GET_FILE_MAP") == 0) {
        out_request->cmd = METADATA_CMD_GET_FILE_MAP;
        sanitize(out_request->filename, filename, sizeof(out_request->filename));
        return 0;
    }

    if (strcmp(cmd, "WRITE_BLOCK") == 0 || strcmp(cmd, "LOOKUP") == 0) {
        if (parsed < 4 || strcmp(keyword, "BLOCK") != 0) {
            return -1;
        }
        out_request->block_id = block_id;
        sanitize(out_request->filename, filename, sizeof(out_request->filename));
        out_request->cmd = (strcmp(cmd, "WRITE_BLOCK") == 0) ? METADATA_CMD_WRITE_BLOCK : METADATA_CMD_LOOKUP_BLOCK;
        return 0;
    }

    return -1;
}
