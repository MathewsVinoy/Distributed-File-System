#include "load_metadata.h"
#include "common/logging.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int findlocation(const char *metadata_path, const char *input, FileMap *out) {
    if (!metadata_path || !input || !out) {
        return -1;
    }

    FILE *metadata = fopen(metadata_path, "r");
    if (!metadata) {
        LOG_ERROR("Meta Data file not found: %s", metadata_path);
        return -1;
    }

    memset(out, 0, sizeof(*out));

    char buffer[256];
    char label[50];
    char value[256];
    int current_block = -1;
    int found = 0;

    while (fgets(buffer, sizeof(buffer), metadata)) {
        if (sscanf(buffer, "%49s %[\n]", label, value) != 2) {
            continue;
        }

        if (strcmp(value, input) == 0) {
            found = 1;
            strncpy(out->filename, value, MAX_FILENAME_LEN - 1);
            out->filename[MAX_FILENAME_LEN - 1] = '\0';
        }

        if (found) {
            if (strcmp(label, "no_block") == 0) {
                out->total_blocks = atoi(value);
            } else if (strcmp(label, "blockid") == 0) {
                current_block = atoi(value);
                if (current_block >= 0 && current_block < MAX_BLOCKS) {
                    out->blocks[current_block].blockid = current_block;
                }
            } else if (strcmp(label, "locations") == 0 && current_block >= 0) {
                char *token = strtok(value, "{},");
                int j = 0;
                while (token && j < MAX_DS) {
                    strncpy(out->blocks[current_block].locations[j], token, MAX_ADDR_LEN - 1);
                    out->blocks[current_block].locations[j][MAX_ADDR_LEN - 1] = '\0';
                    token = strtok(NULL, "{},");
                    j++;
                }
            } else if (strcmp(label, "ports") == 0 && current_block >= 0) {
                char *token = strtok(value, "{},");
                int j = 0;
                while (token && j < MAX_DS) {
                    out->blocks[current_block].ports[j] = atoi(token);
                    token = strtok(NULL, "{},");
                    j++;
                }
            }
        }
    }

    fclose(metadata);
    return found ? 0 : -1;
}

