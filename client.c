#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>

typedef struct {
    int blockid;
    char location[50];
    int port;
} block;

int main(){
    int clint_socket = socket(AF_INET, SOCK_STREAM,0);
    if(clint_socket==-1){
        perror("Socket cretaion failed");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(clint_socket,(struct sockaddr *)&server_addr, sizeof(server_addr))==-1){
        perror("Socket cretaion failed");
        close(clint_socket);
        exit(EXIT_FAILURE);
    }
    char comment[100] = "LOOKUP my_file.txt BLOCK 0";
    send(clint_socket,comment,sizeof(comment),0);
    block location;
    recv(clint_socket, &location, sizeof(location), 0);
    printf("The location of the data server is: %d\n", location.port);

    close(clint_socket);
    

    // connection to the dataserver for getting data
    clint_socket = socket(AF_INET, SOCK_STREAM,0);
    if(clint_socket==-1){
        perror("Socket cretaion failed");
        exit(EXIT_FAILURE);
    } 
    server_addr.sin_port =htons(location.port);
    server_addr.sin_addr.s_addr = inet_addr(location.location);

    if(connect(clint_socket,(struct sockaddr *)&server_addr, sizeof(server_addr))==-1){
        perror("Socket cretaion failed");
        close(clint_socket);
        exit(EXIT_FAILURE);
    }

    free(comment);

    comment = "GET_BLOCK 0";
    send(clint_socket,comment,sizeof(comment),0);
    char *buffer;
    recv(clint_socket,buffer, sizeof(buffer),0);
    printf("%s",buffer);



 
    return 0;

}
