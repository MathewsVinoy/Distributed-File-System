#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "load_metadata.h"
#include "datastruct.h"
#include "metadata/getlocation.h"
#include "clint/connction_cash.h"

int writeblock (int server_fd, char filename[100],int no){
    FileMapchar buffer[100], msg[50];
    char buffer[100], msg[100];
    int i,j,about=1;
    blockinfo block;
    for(i=0;i<full_map.total_blocks;i++){
        if(full_map.blocks[i].blockid==no){
            for(j=0;j<sizeof(full_map.blocks[i].locations),j++){
                int clint_socket = get_connection(full_map.blocks[i].locations[j],full_map.blocks[i].ports[j]);
                sprintf(msg,"PREPARE_WRITE Block %d",no);
                send(clint_socket,msg,sizeof(msg),0);
                ssize_t recv_bytes = recv(clint_socket, buffer, sizeof(buffer) - 1, 0);
                if (recv_bytes <= 0) {
                    close(clint_socket);
                    return 0
                }
                if(strcmp(buffer, "ABORT")==0){
                    close(clint_socket);
                    return 0;
                }
            }
        }
    }
}

