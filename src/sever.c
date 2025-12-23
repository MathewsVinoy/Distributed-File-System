#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define DATA_FILE "database/my_file.txt"
#define BLOCK_SIZE 1024

int main(int argc, char *argv[]){
    int temp_conn;
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
        perror("Bind Failed");
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
            continue;
        }
    

        char buffer[BLOCK_SIZE];
        char request[128];
        ssize_t req_len = recv(clint_sock, request, sizeof(request) - 1, 0);
        if (req_len <= 0) {
            perror("recv failed for request");
            close(clint_sock);
            continue;
        }
        request[req_len] = '\0';
        printf("request: %s\n", request);
        char command[16] = {0};
        char keyword[16] = {0};
        int block_id = -1;
        int parsed = sscanf(request, "%15s %15s %d", command, keyword, &block_id);

        if (parsed < 2 || strcmp(keyword, "BLOCK") != 0) {
            const char *msg = "ERROR: Invalid request format";
            send(clint_sock, msg, strlen(msg), 0);
            close(clint_sock);
            continue;
        }

        if (strcmp(command, "GET") == 0) {
            if (parsed < 3 || block_id < 0) {
                const char *msg = "ERROR: Missing block id";
                send(clint_sock, msg, strlen(msg), 0);
                close(clint_sock);
                continue;
            }

            FILE *fp = fopen(DATA_FILE, "rb");
            if (fp == NULL) {
                perror("Failed to open file for GET");
                const char *msg = "ERROR: File open failed";
                send(clint_sock, msg, strlen(msg), 0);
            } else {
                if (fseek(fp, (long)block_id * BLOCK_SIZE, SEEK_SET) != 0) {
                    perror("fseek failed");
                }

                size_t r = fread(buffer, 1, BLOCK_SIZE, fp);
                if (r == 0 && ferror(fp)) {
                    perror("fread failed");
                } else {
                    send(clint_sock, buffer, r, 0);
                }
                fclose(fp);
            }
        }else if(strcmp(command, "PREPARE_WRITE")==0){
            temp_conn = clint_sock;
            close(clint_sock);
            int clint_sock = accept(server_fd,(struct sockaddr *)&client_addr, &clint_len);
            if(clint_sock<0){
                perror("Accept Failed");
                send(temp_conn,"ABORT",strlen("ABORT"),0);
                close(temp_conn);
            }
            ssize_t req_len = recv(clint_sock, request, sizeof(request) - 1, 0);
            if (req_len <= 0) {
                perror("recv failed for request");
                send(temp_conn,"ABORT",strlen("ABORT"),0);
                close(clint_sock);
                close(temp_conn);
            }
            request[req_len] = '\0';
            printf("request: %s\n", request);
            sscanf(request, "%15s %15s %d", command, keyword, &block_id);
            char file_path[50];
            sprintf(file_path,"database/log/temp%d.txt",port);
            if(strcmp(command,"PUT")==0){
                FILE fp* = fopen(file_path,'w');
                if (fp == NULL) {
                    perror("Failed to open file for PUT");
                    const char *msg = "ERROR: File open failed";
                    send(clint_sock, msg, strlen(msg), 0);
                    send(temp_conn,"ABORT",strlen("ABORT"),0); 
                } else {
                    if (fseek(fp, (long)block_id * BLOCK_SIZE, SEEK_SET) != 0) {
                        perror("fseek failed");
                        send(temp_conn,"ABORT",strlen("ABORT"),0);
                    }

                    ssize_t recv_data = recv(clint_sock, buffer, sizeof(buffer), 0);
                    if (recv_data <= 0) {
                        perror("recv failed for data");
                        send(temp_conn,"ABORT",strlen("ABORT"),0);
                    } else {
                        if ((size_t)recv_data != fwrite(buffer, 1, (size_t)recv_data, fp)) {
                            perror("fwrite failed");
                            send(temp_conn,"ABORT",strlen("ABORT"),0);
                        }
                        send(clint_sock, "ok", 2, 0);
                        send(temp_conn, "READY", 5,0);
                    }
                }
            }
            req_len=recv(temp_conn, request, sizeof(request) - 1, 0);
            request[req_len] = '\0';
            printf("request: %s\n", request);
            sscanf(request, "%15s %15s %d", command, keyword, &block_id);
            if(strcmp(command,"GLOBAL_COMMIT")==0){
                FILE fp1* = fopen(DATA_FILE, "wb");
                FILE fp2* = fopen(file_path, "rb");
                
            }else{

            }

        }else {
            const char *msg = "ERROR: Unknown command";
            send(clint_sock, msg, strlen(msg), 0);
        }

        close(clint_sock);
        printf("Client connection closed\n");
    }
    close(server_fd);
    return 0;

}


