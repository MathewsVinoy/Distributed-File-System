#include "clint/lookup.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include "datastruct.h"
#include "clint/connction_cash.h"

int lookupBlock(){
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

    char comment[100] = "LOOKUP my_file.txt BLOCK 0";
    // char comment[100] = "WRITE_BLOCK my_file.txt BLOCK1";
    
    send(clint_socket, comment, strlen(comment), 0);

    blockinfo location;
    recv(clint_socket, &location, sizeof(location), 0);
    

    close(clint_socket);

    // Connect to the data server
    size_t server_count = sizeof(location.locations) / sizeof(location.locations[0]);
    for(size_t i = 0; i < server_count; i++){

        clint_socket = socket(AF_INET, SOCK_STREAM, 0);
        if(clint_socket == -1){
            perror("Socket creation failed");
            continue;
        }

        server_addr.sin_port = htons(location.ports[i]);
        server_addr.sin_addr.s_addr = inet_addr(location.locations[i]);

        if(connect(clint_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
            perror("Data server connection failed");
            close(clint_socket);
            continue;
        }

        // Correct way to send GET request
        char request[50] = "GET_BLOCK 0";
        send(clint_socket, request, strlen(request), 0);

        char buffer[1024];  // allocate proper space
        memset(buffer, 0, sizeof(buffer));
        ssize_t n =recv(clint_socket, buffer, sizeof(buffer), 0);

        if (n > 0) {
            buffer[n] = '\0';
            printf("Received data: %s\n", buffer);
            close(clint_socket);
            break;
        }else {
            perror("Failed to receive data");
            close(clint_socket);
            continue; // Try next DS
        }
    }
    return 0;
}

