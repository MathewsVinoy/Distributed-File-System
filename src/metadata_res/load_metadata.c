#include "load_metadata.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

FileMap findlocation(char* input) {
    FILE *metadata;
    char buffer[256];
    char label[50], value[256];
    FileMap out = {0};
    int j;

    metadata = fopen("database/metadata.txt", "r");
    if (!metadata) {
        perror("Meta Data file not found!!");
        return out;
    }

    int flag = 0, current_block = -1;

    while (fgets(buffer, sizeof(buffer), metadata)) {
        sscanf(buffer, "%s %[^\n]", label, value);

        if (strcmp(value, input) == 0) {
            flag = 1;
            strncpy(out.filename, value, MAX_FILENAME_LEN - 1);
            out.filename[MAX_FILENAME_LEN - 1] = '\0';
        }

        if (flag == 1) {
            if (strcmp(label, "no_block") == 0) {
                out.total_blocks = atoi(value);
            } else if (strcmp(label, "blockid") == 0) {
                current_block = atoi(value);
                if (current_block >= 0 && current_block < MAX_BLOCKS) {
                    out.blocks[current_block].blockid = current_block;
                }
            } else if (strcmp(label, "locations") == 0) {
                char *token = strtok(value, "{},");
                j = 0;
                while (token != NULL && j < MAX_DS) {
                    strncpy(out.blocks[current_block].locations[j], token, MAX_ADDR_LEN - 1);
                    out.blocks[current_block].locations[j][MAX_ADDR_LEN - 1] = '\0';
                    token = strtok(NULL, "{},");
                    j++;
                }
            } else if (strcmp(label, "ports") == 0) {
                char *token = strtok(value, "{},");
                j = 0;
                while (token != NULL && j < MAX_DS) {
                    out.blocks[current_block].ports[j] = atoi(token);
                    token = strtok(NULL, "{},");
                    j++;
                }
            }
        }
    }

    fclose(metadata);
    return out;
}

