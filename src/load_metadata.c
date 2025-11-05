#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_BLOCKS 10
#define MAX_FILENAME_LEN 50
#define MAX_ADDR_LEN 50
#define MAX_DS 50

typedef struct {
    int blockid;
    char locations[MAX_DS][MAX_ADDR_LEN];
    int ports[MAX_DS];
} blockinfo;

typedef struct {
    char filename[MAX_FILENAME_LEN];
    int total_blocks;
    blockinfo blocks[MAX_BLOCKS];
} FileMap;

int main() {
    FILE *metadata;
    char buffer[256]; // Allocate buffer
    char label[50], value[50];
    FileMap out;
    int i, j;

    metadata = fopen("../build/metadata.txt", "r");
    if (!metadata) {
        perror("Meta Data file not found!!");
        return 1;
    }

    char input[100] = "my_file.txt";
    int flag = 0, count = 0;
    int current_block = -1;

    while (fgets(buffer, sizeof(buffer), metadata)) {
        printf("%s", buffer);
        sscanf(buffer, "%s %[^\n]", label, value);

        if (strcmp(value, input) == 0) {
            flag = 1;
            strcpy(out.filename, value);
        }

        if (flag == 1) {
            if (strcmp(label, "no_block") == 0) {
                out.total_blocks = atoi(value);
            } else if (strcmp(label, "blockid") == 0) {
                current_block = atoi(value);
                out.blocks[current_block].blockid = current_block;
            } else if (strcmp(label, "locations") == 0) {
                // Parse locations (simplified: assumes format "{ip1,ip2,...}")
                char *token = strtok(value, "{},");
                j = 0;
                while (token != NULL && j < MAX_DS) {
                    strcpy(out.blocks[current_block].locations[j], token);
                    token = strtok(NULL, "{},");
                    j++;
                }
            } else if (strcmp(label, "ports") == 0) {
                // Parse ports (simplified: assumes format "{port1,port2,...}")
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

    // Print the parsed data for verification
    printf("Filename: %s\n", out.filename);
    printf("Total Blocks: %d\n", out.total_blocks);
    for (i = 0; i < out.total_blocks; i++) {
        printf("Block ID: %d\n", out.blocks[i].blockid);
        for (j = 0; j < MAX_DS && strlen(out.blocks[i].locations[j]) > 0; j++) {
            printf("  Location: %s, Port: %d\n",
                   out.blocks[i].locations[j],
                   out.blocks[i].ports[j]);
        }
    }

    fclose(metadata);
    return 0;
}

