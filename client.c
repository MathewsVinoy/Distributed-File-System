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
    server_addr.sin_port = htons(8081);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(clint_socket,(struct sockaddr *)&server_addr, sizeof(server_addr))==-1){
        perror("Socket cretaion failed");
        close(clint_socket);
        exit(EXIT_FAILURE);
    }

    char msg[1024];

    size_t buffer = recv(clint_socket, msg, 1024,0);
    
    for (size_t i = 0; i < buffer; i++) {
        putchar(msg[i]);
    }
    putchar('\n');
    // send(clint_socket, msg, strlen(msg), 0);
    //  
    // recv(clint_socket,buffer,sizeof(buffer),0);
    // printf("server message:  %s\n",buffer);
    //
    // int i=0,input;
    // while(i!=1){
    //     printf("\nEnter an number for the following operations: \n1.Download\n2.Upload\n3.Exit\nEnter the number: ");
    //     scanf("%d",&input);
    //     write(clint_socket, &input, sizeof(input));
    //     if(input == 3){
    //         close(clint_socket);
    //         i=1;
    //     }
    //    }

    close(clint_socket);
    return 0;

}
