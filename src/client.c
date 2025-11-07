#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include "datastruct.h"

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
    
    send(clint_socket, comment, strlen(comment), 0);

    FileMap datamap;  
    recv(clint_socket, &datamap, sizeof(datamap), 0);
    

    close(clint_socket);
    return 0;
}

