
#ifndef DATASTRUCT_H
#define DATASTRUCT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_BLOCKS
#define MAX_BLOCKS 10
#endif

#ifndef MAX_FILENAME_LEN
#define MAX_FILENAME_LEN 50
#endif

#ifndef MAX_ADDR_LEN
#define MAX_ADDR_LEN 50
#endif

#ifndef MAX_DS
#define MAX_DS 50
#endif

typedef struct blockinfo {
    int blockid;
    char locations[MAX_DS][MAX_ADDR_LEN];
    int ports[MAX_DS];
} blockinfo;

typedef struct FileMap {
    char filename[MAX_FILENAME_LEN];
    int total_blocks;
    blockinfo blocks[MAX_BLOCKS];
} FileMap;

typedef struct ConnectionPool {
    int port;
    char location[MAX_ADDR_LEN];
    int socket_fd;
    struct ConnectionPool *next;
} ConnectionPool;

#ifdef __cplusplus
}
#endif

#endif /* DATASTRUCT_H */

