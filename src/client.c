#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include "datastruct.h"
#include "clint/connction_cash.h"


int main(){
    int meta_socket = socket(AF_INET, SOCK_STREAM, 0);
    printf("\t\tTHIS IS AN CLINT CONNECTION.");
    if(meta_socket == -1){
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(meta_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
        perror("Metadata server connection failed");
        close(meta_socket);
        exit(EXIT_FAILURE);
    }

    char comment[100] = "GET_FILE_MAP my_file.txt";

    FILE *fp1 = fopen("out/cli/myfile.txt", "a");
    
    send(meta_socket, comment, strlen(comment), 0);

    FileMap datamap;  
    recv(meta_socket, &datamap, sizeof(datamap), 0);
    printf("Received file map for %s with %d blocks\n", datamap.filename, datamap.total_blocks);

    for(int i =0; i<datamap.total_blocks;i++){
        for(size_t j=0;j<sizeof(datamap.blocks[i].locations);j++){
            int clint_socket = get_connection(datamap.blocks[i].locations[j],datamap.blocks[i].ports[j]);
            char request[100];
            sprintf(request , "%s %d","GET BLOCK",datamap.blocks[i].blockid);
            send(clint_socket, request, strlen(request), 0);

            char buffer[1024];  
            memset(buffer, 0, sizeof(buffer));
            ssize_t n =recv(clint_socket, buffer, sizeof(buffer), 0);

            if (n > 0) {
                buffer[n] = '\0';
                printf("Received data: %s\n", buffer);
                fprintf(fp1, "%s", buffer);
                release_connection(clint_socket);
                break;
            }else {
                perror("Failed to receive data");
                release_connection(clint_socket);
                continue;
            }
        } 
    }

    close(meta_socket);
    close_all_connections();
    fclose(fp1);
    return 0;
}

