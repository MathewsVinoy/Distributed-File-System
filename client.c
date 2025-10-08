#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>

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
    char* location;
    recv(clint_socket,location, 1024,0);
    printf("The location of the dataserver is : %s",location);

    char msg[1024];

    size_t buffer = recv(clint_socket, msg, 1024,0);
    
    for (size_t i = 0; i < buffer; i++) {
        putchar(msg[i]);
    }
    putchar('\n');
   

    close(clint_socket);
    return 0;

}
