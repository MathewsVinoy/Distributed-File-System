#include <string.h>
#include "metadata/getlocation.h"
#include "datastruct.h"
#include "load_metadata.h"

int get_location(const char *metadata_path, const char *filename, int block_id, blockinfo *out) {
    if (!metadata_path || !filename || !out) {
        return -1;
    }

    FileMap file_map;
    if (findlocation(metadata_path, filename, &file_map) != 0) {
        return -1;
    }

    if (strcmp(file_map.filename, filename) != 0) {
        return -1;
    }

    for (int j = 0; j < file_map.total_blocks; j++) {
        if (file_map.blocks[j].blockid == block_id) {
            memcpy(out, &file_map.blocks[j], sizeof(blockinfo));
            return 0;
        }
    }
    return -1;
}

