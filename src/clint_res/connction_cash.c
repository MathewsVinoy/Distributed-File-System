#include "clint/connction_cash.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include "datastruct.h"

ConnectionPool *connCash = NULL;


int lookup_connection(const char ip[50], int port){
    ConnectionPool *current = connCash;
    while(current != NULL){
        if(strcmp(current->location,ip)==0 && current->port==port){
            return current->socket_fd;
        }
        current = current->next;
    }
    return -1;
}

int get_connection(const char ip[50], int port){
    int clint_socket = lookup_connection(ip,port);
    if(clint_socket !=-1){
        return clint_socket;
    }
    clint_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(clint_socket == -1){
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if(inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0){
        perror("Invalid IP address");
        close(clint_socket);
        return -1;
    }

    if(connect(clint_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
        perror("Data server connection failed");
        close(clint_socket);
        return -1;
    }
    ConnectionPool *new_node = (ConnectionPool *)malloc(sizeof(ConnectionPool));
    new_node->socket_fd = clint_socket;
    strcpy(new_node->location, ip);
    new_node->port = port;
    new_node->next = connCash;
    connCash = new_node;

    return clint_socket;

}

int release_connection(int socket_fd) {
    ConnectionPool *current = connCash;
    ConnectionPool *previous = NULL;

    while (current != NULL) {
        if (current->socket_fd == socket_fd) {
            if (previous != NULL) {
                previous->next = current->next;
            } else {
                connCash = current->next;
            }
            close(socket_fd);
            free(current);
            return 0;
        }
        previous = current;
        current = current->next;
    }

    /* socket not tracked; ensure it is closed anyway */
    close(socket_fd);
    return -1;
}

void close_all_connections() {
    ConnectionPool *current = connCash;
    while (current != NULL) {
        close(current->socket_fd);
        ConnectionPool *temp = current;
        current = current->next;
        free(temp);
    }
    connCash = NULL;
}


