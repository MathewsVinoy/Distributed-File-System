#include "clint/writefuncs.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include "datastruct.h"
#include "clint/connction_cash.h"


int writeFile(){
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

    //char comment[100] = "LOOKUP my_file.txt BLOCK 0";
    char comment[100] = "WRITE_BLOCK my_file.txt BLOCK 1";
    
    send(clint_socket, comment, strlen(comment), 0);

    blockinfo location;
    ssize_t recv_bytes = recv(clint_socket, &location, sizeof(location), 0);
    if (recv_bytes <= 0) {
        perror("Failed to receive location from metadata server");
        close(clint_socket);
        exit(EXIT_FAILURE);
    }

    close(clint_socket);

    //test write the operation 
    char test[1024] = "\ntest Data writing to file"; 
    // Connect to the data server
    /* iterate over returned locations until an empty address or port 0 is found */
    for(int i=0; i < 50; i++){
        if (location.ports[i] == 0 || location.locations[i][0] == '\0') {
            break;
        }
        clint_socket = socket(AF_INET, SOCK_STREAM, 0);
        if(clint_socket == -1){
            perror("Socket creation failed");
            continue;
        }

        server_addr.sin_port = htons(location.ports[i]);
        server_addr.sin_addr.s_addr = inet_addr(location.locations[i]);
        printf("Location ==> %s \n Port ==> %d\n",location.locations[i], location.ports[i]);

        if(connect(clint_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
            perror("Data server connection failed");
            close(clint_socket);
            /* try next location instead of aborting all */
            continue;
            // continue;
        }

        // Correct way to send GET request
        char request[50] = "PUT BLOCK 1";
        send(clint_socket, request, strlen(request), 0);

        char buffer[1024];  // allocate proper space
        memset(buffer, 0, sizeof(buffer));
        recv(clint_socket, buffer, sizeof(buffer), 0);
        printf("After put block1: %s",buffer);

       if (strcmp(buffer, "ok") == 0) {
            send(clint_socket, test, strlen(test), 0);
            memset(buffer, 0, sizeof(buffer));
            ssize_t recv_bytes = recv(clint_socket, buffer, sizeof(buffer) - 1, 0);
            if (recv_bytes <= 0) {
                perror("Failed to receive confirmation");
                close(clint_socket);
                continue;
            }
            buffer[recv_bytes] = '\0';
            if (strcmp(buffer, "ok") == 0) {
                printf("Data written successfully\n");
            } else {
                perror("Error occurred when writing");
            }
        } else {
            perror("Failed to write the data");
        }

        close(clint_socket);
    }    
    return 0;
}

