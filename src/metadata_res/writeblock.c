#include "metadata/writeblock.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "load_metadata.h"
#include "datastruct.h"
#include "clint/connction_cash.h"

static int socket_addrs[100];
static int socket_count = 0;

int* write_block(FileMap datamap, int blockid){
    char msg[100];
    socket_count = 0;
    sprintf(msg, "PREPARE_WRITE BLOCK %d",blockid);
    for(int i = 0; i < datamap.total_blocks; i++){
        if(datamap.blocks[i].blockid == blockid){
            for(int j = 0; j < MAX_DS; j++){
                if(datamap.blocks[i].ports[j] == 0 || datamap.blocks[i].locations[j][0] == '\0'){
                    break;
                }
                int clint_socket = get_connection(datamap.blocks[i].locations[j], datamap.blocks[i].ports[j]);
                if(clint_socket >= 0){
                    send(clint_socket, msg, strlen(msg), 0);
                    socket_addrs[socket_count] = clint_socket;
                    socket_count++;
                }
            }
            break;
        }
    }
    //send completed 
    return socket_addrs;
}

void status_listen(int *ds_lists, int blockid){
    extern int socket_count;
    char buffer[50], replay[50];
    int flag = 0;
    for(int i = 0; i < socket_count; i++){
        memset(buffer, 0, sizeof(buffer));
        recv(ds_lists[i], buffer, sizeof(buffer), 0);
        if(strcmp("ABORT", buffer) == 0){
            flag = 1;
            break;
        }
    }
    for(int i = 0; i < socket_count; i++){
        if(flag == 1){
            sprintf(replay, "GLOBAL_ABORT BLOCK %d", blockid);
            send(ds_lists[i], replay, strlen(replay), 0);
        }else{
            sprintf(replay, "GLOBAL_COMMIT BLOCK %d", blockid);
            send(ds_lists[i], replay, strlen(replay), 0);
        }
        close(ds_lists[i]);
    }
}
