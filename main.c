#include <stdio.h>
#include <arpa/inet.h> 
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> 
#include <sys/socket.h>
static int globid =0;

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
    printf("Checkmate");
    return 0;
}

int globidGenerater(){
    return globid++;
}
