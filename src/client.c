#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include "datastruct.h"

int main(){
    /* socket used to talk to metadata server */
    int meta_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (meta_sock == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(meta_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Metadata server connection failed");
        close(meta_sock);
        exit(EXIT_FAILURE);
    }

    char comment[100] = "GET_FILE_MAP my_file.txt";
    // char comment[100] = "WRITE_BLOCK my_file.txt BLOCK1";
    
    send(meta_sock, comment, strlen(comment), 0);

    FileMap datamap;
    ssize_t got = recv(meta_sock, &datamap, sizeof(datamap), 0);
    if (got <= 0) {
        if (got == 0) fprintf(stderr, "Metadata server closed connection\n");
        else perror("recv from metadata server failed");
        close(meta_sock);
        exit(EXIT_FAILURE);
    }

    /* For each block, try the data servers that were returned. Use the
       number of entries in the locations array, validate addresses with
       inet_pton, skip zero ports, and handle recv return values correctly. */
    for (int i = 0; i < datamap.total_blocks; i++) {
        size_t ds_count = sizeof(datamap.blocks[i].locations) / sizeof(datamap.blocks[i].locations[0]);
        for (size_t j = 0; j < ds_count; j++) {
            int port = datamap.blocks[i].ports[j];
            char *addr_str = datamap.blocks[i].locations[j];

            if (port == 0) continue;
            if (addr_str[0] == '\0') continue;

            int data_sock = socket(AF_INET, SOCK_STREAM, 0);
            if (data_sock == -1) {
                perror("Socket creation failed");
                continue;
            }

            struct sockaddr_in ds_addr = {0};
            ds_addr.sin_family = AF_INET;
            ds_addr.sin_port = htons(port);

            if (inet_pton(AF_INET, addr_str, &ds_addr.sin_addr) != 1) {
                fprintf(stderr, "Invalid data server address: %s\n", addr_str);
                close(data_sock);
                continue;
            }

            if (connect(data_sock, (struct sockaddr *)&ds_addr, sizeof(ds_addr)) == -1) {
                perror("Data server connection failed");
                close(data_sock);
                continue;
            }

            char request[100];
            snprintf(request, sizeof(request), "GET_BLOCK %d", datamap.blocks[i].blockid);
            send(data_sock, request, strlen(request), 0);

            char buffer[1024];
            memset(buffer, 0, sizeof(buffer));
            ssize_t n = recv(data_sock, buffer, sizeof(buffer) - 1, 0);

            if (n > 0) {
                buffer[n] = '\0';
                printf("Received data: %s\n", buffer);
                close(data_sock);
                break; /* got this block */
            } else if (n == 0) {
                fprintf(stderr, "Connection closed by data server %s:%d\n", addr_str, port);
                close(data_sock);
                continue; /* try next DS */
            } else {
                perror("Failed to receive data");
                close(data_sock);
                continue; /* try next DS */
            }
        }
    }

    close(meta_sock);

    close(clint_socket);
    return 0;
}

