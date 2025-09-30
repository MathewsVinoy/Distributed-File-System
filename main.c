#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
static int globid =0;

#define PORT 8080
#define MAX_CONN = 5

struct DataNode{
    int id;
    char ip_addr[100];
    int port;
};
struct Block{
    int bId;
    DtaNode* address[100];
};
stuct fileMetadata{
    char filename[100];
    Block* subids[100];
};

int main(){ 
    int server_fd, new_socket;
    struct sockaddr_in , address;
    int addrlen = sizeof(address);

    if((server_fd = socket(AF_INET, SOCK_STREAM,0))==0){
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    printf("Checkmate");
    return 0;
}

int globidGenerater(){
    return globid++;
}
