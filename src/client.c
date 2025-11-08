#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include "datastruct.h"

ConnectioPool *connCash = NULL;

int get_connection(const char ip[50], int port){
    clint_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(clint_socket == -1){
        return -1
    }

    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    if(connect(clint_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
        perror("Data server connection failed");
        close(clint_socket);
        return -1;
    }
    ConnectionPool *new_node = (ConnectionPool *)malloc(sizeof(ConnectionPool));

}

int main(){
    int clint_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(clint_socket == -1){
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(clint_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
        perror("Metadata server connection failed");
        close(clint_socket);
        exit(EXIT_FAILURE);
    }

    char comment[100] = "GET_FILE_MAP my_file.txt";
    // char comment[100] = "WRITE_BLOCK my_file.txt BLOCK1";

    FILE *fp1 = fopen("out/cli/myfile.txt", "a");
    
    send(clint_socket, comment, strlen(comment), 0);

    FileMap datamap;  
    recv(clint_socket, &datamap, sizeof(datamap), 0);
    for(int i =0; i<datamap.total_blocks;i++){
        for(size_t j=0;j<sizeof(datamap.blocks[i].locations);j++){
                        char request[100];
            sprintf(request , "%s %d","GET BLOCK",datamap.blocks[i].blockid);
            send(clint_socket, request, strlen(request), 0);

            char buffer[1024];  // allocate proper space
            memset(buffer, 0, sizeof(buffer));
            ssize_t n =recv(clint_socket, buffer, sizeof(buffer), 0);

            if (n > 0) {
                buffer[n] = '\0';
                printf("Received data: %s\n", buffer);
                fprintf(fp1, "%s", buffer);
                close(clint_socket);
                break;
            }else {
                perror("Failed to receive data");
                close(clint_socket);
                continue; // Try next DS
            }
        } 
    }

    close(clint_socket);
    return 0;
}

