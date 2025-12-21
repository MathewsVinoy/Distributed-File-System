#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "load_metadata.h"
#include "datastruct.h"
#include "clint/connction_cash.h"

int send_(int server_fd, FileMap full_map,int no,char msg[100]){
    FileMapchar buffer[100], msg[50];
    char buffer[100];
    int i,j;
    for(i=0;i<full_map.total_blocks;i++){
        if(full_map.blocks[i].blockid==no){
            for(j=0;j<sizeof(full_map.blocks[i].locations),j++){
                int clint_socket = get_connection(full_map.blocks[i].locations[j],full_map.blocks[i].ports[j]);                     send(clint_socket,msg,sizeof(msg),0);
                ssize_t recv_bytes = recv(clint_socket, buffer, sizeof(buffer) - 1, 0);
                if (recv_bytes <= 0) {
                    close(clint_socket);
                    return 1;
                }
                if(strcmp(buffer, "ABORT")==0){
                    close(clint_socket);
                    return 1;
                }
            }
        }
    }
    return 0;
}

void write_block(int server_fd, FileMap full_map,int no){
    char msg[100];
    int n;
    sprintf(msg,"PREPARE_WRITE BLOCK %d",no);
    int about=send_msg(server_fd,full_map,no,msg);
    if(about==1){
        sprintf(msg,"GLOBAL_ABORT BLOCK %d",no);
        n=send_mg(server_fd,full_map,no,msg);
    }else{
        sprintf(msg,"GLOBAL_COMMIT BLOCK %d",no);
        n=send_mg(server_fd,full_map,no,msg);
    }
}



