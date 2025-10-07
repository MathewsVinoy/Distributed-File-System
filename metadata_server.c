#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define MAX_BLOCKS 10
#define MAX_FILENAME_LEN 50
#define MAX_ADDR_LEN 50



trypedef struct {
    int blockid;
    char location[MAX_ADDR_LEN];
} blockinfo;

trypedef struct {
    char filename[MAX_FILENAME_LEN];
    int noblocks;
    blockinfo blocks[MAX_BLOCKS];
}FileMap;


//for debugging
FileMap file_map[] = {
    {
        .filename = "my_file.txt",
        .total_blocks = 2,
        .blocks = {
            { .block_id = 0, .location = "Data_Server_A:8000" },
            { .block_id = 1, .locati:n = "Data_Server_B:8001" }
        }
    }
};


int main(){
    int server_fd = socket(AF_INET,SOCK_STREAM,0);
    if(server_fd <0){
        perror("Socket connection failed");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(9000);

    if(bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("Blint Failed");
        exit(EXIT_FAILURE);

    }
    
    if(listen(server_fd, 5)<0){
        perror("Listen Failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in client_addr;
    socklen_t clint_len = sizeof(client_addr);
    int clint_sock = accept(server_fd,(struct sockaddr *)&client_addr, &clint_len);
    if(clint_sock<0){
        perror("Accept Failed");
        exit(EXIT_FAILURE);
    }

    return 0;
}
