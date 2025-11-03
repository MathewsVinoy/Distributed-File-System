#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAX_BLOCKS 10
#define MAX_FILENAME_LEN 50
#define MAX_ADDR_LEN 50
#define MAX_DS 50

typedef struct {
    int blockid;
    char locations[MAX_ADDR_LEN][MAX_DS];
    int ports[MAX_DS];
} blockinfo;

typedef struct {
    char filename[MAX_FILENAME_LEN];
    int total_blocks;
    blockinfo blocks[MAX_BLOCKS];
} FileMap;

FileMap file_map[] = {
    {
        .filename = "my_file.txt",
        .total_blocks = 2,
        .blocks = {
            { .blockid = 0, .locations = {"127.0.0.1","127.0.0.1"}, .ports = {8000, 8001} },
            { .blockid = 1, .locations = {"127.0.0.1"}, .ports= {8001} }
        }
    }
};
int file_map_count = sizeof(file_map) / sizeof(file_map[0]);

blockinfo get_location(char* filename, int block_id){
    for(int i = 0; i < file_map_count; i++){
        if(strcmp(file_map[i].filename, filename) == 0){
            for(int j = 0; j < file_map[i].total_blocks; j++){
                if(file_map[i].blocks[j].blockid == block_id){
                    return file_map[i].blocks[j];
                }
            }
        }
    }
    /* return a zero-initialized blockinfo to avoid returning uninitialized memory */
    blockinfo b;
    memset(&b, 0, sizeof(b));
    b.blockid = -1;
    return b;
}

void split(const char* input, char* filename, int* block_id){
    char command[20], block_name[20];
    /* default in case parsing fails */
    *block_id = -1;
    sscanf(input, "%19s %49s %19s %d", command, filename, block_name, block_id);
}

int main(){
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0){
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(9000);

    if(bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if(listen(server_fd, 5) < 0){
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Metadata server listening on port 9000...\n");

    char buffer[1024];
    struct sockaddr_in client_addr;
    socklen_t clint_len = sizeof(client_addr);

    while(1){
        int clint_sock = accept(server_fd, (struct sockaddr *)&client_addr, &clint_len);
        if(clint_sock < 0){
            perror("Accept failed");
            continue;
        }

        memset(buffer, 0, sizeof(buffer));
        recv(clint_sock, buffer, sizeof(buffer), 0);
        printf("Received request: %s\n", buffer);

        char filename[100];
        int block_id;
        split(buffer, filename, &block_id);

        blockinfo reply = get_location(filename, block_id);
        send(clint_sock, &reply, sizeof(reply), 0);

        close(clint_sock);
    }

    close(server_fd);
    return 0;
}

