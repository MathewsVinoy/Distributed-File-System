#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(int argc, char *argv[]){
    FILE* fp1;
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    if (port <= 0) {
        fprintf(stderr, "Invalid port number\n");
        exit(EXIT_FAILURE);
    }
    int server_fd = socket(AF_INET,SOCK_STREAM,0);
    if(server_fd <0){
        perror("Socket connection failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if(bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("Blint Failed");
        exit(EXIT_FAILURE);

    }
    
    if(listen(server_fd, 5)<0){
        perror("Listen Failed");
        exit(EXIT_FAILURE);
    }
    while(1){
        struct sockaddr_in client_addr;
        socklen_t clint_len = sizeof(client_addr);
        int clint_sock = accept(server_fd,(struct sockaddr *)&client_addr, &clint_len);
        if(clint_sock<0){
            perror("Accept Failed");
            exit(EXIT_FAILURE);
        }
    

        char buffer[1024], request[50];
        ssize_t req_len = recv(clint_sock, request, sizeof(request) - 1, 0);
        if (req_len <= 0) {
            perror("recv failed for request");
            close(clint_sock);
            continue;
        }
        request[req_len] = '\0';
        printf("request: %s\n", request);
        char inst[20], block[10];
        int value;
        sscanf(request, "%s %s %d", inst, block, &value);
        if (strncmp(inst, "GET", strlen("GET")) == 0) {
            fp1 = fopen("build/my_file.txt", "r");
            if (fp1 == NULL) {
                perror("Failed to open file for GET");
            } else {
            fseek(fp1, (value * 1024), SEEK_SET);
            size_t r = fread(buffer, 1, 1024, fp1);
            send(clint_sock, buffer, r, 0);
            printf("send successfully\n");
            }
        }else if (strcmp(request, "PUT BLOCK 1")==0){
            /* use prefix compare in case client sends extra data or different line endings */
            if (strncmp(request, "PUT BLOCK 1", strlen("PUT BLOCK 1")) != 0) {
                /* not the command we expect */
                close(clint_sock);
                continue;
            }
            fp1 = fopen("build/my_file.txt", "a");
            if (fp1 == NULL) {
                perror("Failed to open file");
                send(clint_sock, "ERROR: File open failed", 22, 0);
            } else {
                send(clint_sock, "ok", 2, 0);
                ssize_t recv_data = recv(clint_sock, buffer, sizeof(buffer) - 1, 0);
                if (recv_data <= 0) {
                    perror("recv failed for data");
                } else {
                    buffer[recv_data] = '\0';
                    fprintf(fp1, "%s", buffer);
                    send(clint_sock, "ok", 2, 0);
                }
                
            }    
        }
        close(clint_sock);
        printf("the connection breaked the connection ");
    }
    close(server_fd);
    fclose(fp1); 
    return 0;

}


