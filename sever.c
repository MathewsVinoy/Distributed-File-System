#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(int argc, char *argv[]){
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

    struct sockaddr_in client_addr;
    socklen_t clint_len = sizeof(client_addr);
    int clint_sock = accept(server_fd,(struct sockaddr *)&client_addr, &clint_len);
    if(clint_sock<0){
        perror("Accept Failed");
        exit(EXIT_FAILURE);
    }
    FILE* fp1;
    fp1 = fopen("build/test_data.txt", "r");

    // Use fread and fseek
    // and read from the file
    char buffer[1024], request[50];
    recv(clint_sock,request, sizeof(request),0);
    if (strncmp(request, "GET_BLOCK 0", strlen("GET_BLOCK 0")) == 0) {
        fseek(fp1, 0, SEEK_SET);
        size_t r = fread(buffer, 1, sizeof(buffer), fp1);
        send(clint_sock, buffer, r, 0);
        printf("send successfully\n");
    }else if (strcmp(request, "PUT BLOCK 1"))
    close(clint_sock);
    close(server_fd);
    fclose(fp1);

    return 0;

}


