#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

void userInteraction(int clint_sock);

int main(){
    int server_fd = socket(AF_INET,SOCK_STREAM,0);
    if(server_fd <0){
        perror("Socket connection failed");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8081);

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
    char buffer[1024];
    fseek(fp1, 0, SEEK_SET);
    size_t block1 = fread(buffer, 1, 1024, fp1);
    send(clint_sock, buffer, block1, 0);
    printf("send sucessfully\n");
    for (size_t i = 0; i < block1; i++) {
        putchar(buffer[i]);
    }
    putchar('\n');


    //for sending and resiving data
    // recv(clint_sock,buffer,sizeof(buffer),0);
    // printf("clint message:  %s\n",buffer);
    // if(buffer == "storage"){
    //     char *msg = "Connected to the Server sucessfully";
    //     send(clint_sock, msg, strlen(msg),0);
    // }else if(buffer=="user"){
    //     char *msg = "Hello from the Server";
    //     send(clint_sock, msg, strlen(msg),0);
    //     userInteraction(clint_sock);
    // }
    //
    close(clint_sock);
    close(server_fd);

    return 0;

}

void userInteraction(int clint_sock){
    int res;
    while(1){
        read(clint_sock,&res,sizeof(res));
        printf("The input from the clint is: %d\n",res);
        if(res==3){
            break;
        }else if(res==1){
            printf("The user operation is 1\n");
        }else if(res==2){
            printf("The user operation is 2\n");
        }
    }
}
