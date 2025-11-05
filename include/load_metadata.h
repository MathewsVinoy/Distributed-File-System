#ifndef LOAD_METADATA_H
#define LOAD_METADATA_H

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

// Function prototype
FileMap findlocation(char* input);

#endif // LOAD_METADATA_H

