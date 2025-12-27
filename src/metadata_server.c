#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "load_metadata.h"
#include "datastruct.h"
#include "metadata/getlocation.h"
#include "metadata/writeblock.h"

void split(const char* input, char* filename, int* block_id, char* command){
    /* Robust tokenization: command filename [blockid] */
    char *tmp = strdup(input);
    if (!tmp) {
        *block_id = -1;
        command[0] = '\0';
        filename[0] = '\0';
        return;
    }

    *block_id = -1;
    command[0] = '\0';
    filename[0] = '\0';

    char *tok = strtok(tmp, " \t\r\n");
    if (tok) {
        strncpy(command, tok, 29);
        command[29] = '\0';
    }

    tok = strtok(NULL, " \t\r\n");
    if (tok) {
        strncpy(filename, tok, 99);
        filename[99] = '\0';
    }

    tok = strtok(NULL, " \t\r\n");
    if (tok) {
        *block_id = atoi(tok);
    }

    free(tmp);
}

int main(){
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    FileMap full_map;
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

        char filename[100], command[30];
        int block_id;
        split(buffer, filename, &block_id, command);
        printf("%s - %s\n", filename, command);

        if (strcmp(command, "GET_FILE_MAP") == 0) {
            full_map = findlocation(filename);
            send(clint_sock, &full_map, sizeof(full_map), 0);
        }else if(strcmp(command, "WRITE_BLOCK")==0) {
            full_map = findlocation(filename);
            int *ds_list = write_block(full_map,block_id);
            send(clint_sock,&full_map,sizeof(full_map),0);
            status_listen(ds_list,block_id);
        }
        else {
            blockinfo reply = get_location(filename, block_id);
            printf("Requested block id: %d\n", block_id);
            send(clint_sock, &reply, sizeof(reply), 0);
        }
        close(clint_sock);
    }

    close(server_fd);
    return 0;
}

