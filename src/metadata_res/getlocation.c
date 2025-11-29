#include <string.h>
#include "metadata/getlocation.h"
#include "datastruct.h"
#include "load_metadata.h"


blockinfo get_location(char* filename, int block_id){
    FileMap file_map = findlocation(filename);

    blockinfo b;
    memset(&b, 0, sizeof(b));
    b.blockid = -1;

    /* ensure the returned FileMap matches the requested filename */
    if (strcmp(file_map.filename, filename) != 0) {
        return b;
    }

    for (int j = 0; j < file_map.total_blocks; j++) {
        if (file_map.blocks[j].blockid == block_id) {
            return file_map.blocks[j];
        }
    }

    return b; /* not found */
}

