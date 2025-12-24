#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "load_metadata.h"
#include "datastruct.h"
#include "clint/connction_cash.h"

int* write_block(FileMap datamap, int blockid){
    char msg;
    int n=0, socket_addrs[100];
    sprintf(msg, "PREPARE_WRITE BLOCK %d",blockid);
    for(int i =0; i<datamap.total_blocks;i++){
        if(datamap.blocks[i].blockid==blockid){
            for(size_t j=0;j<sizeof(datamap.blocks[i].locations);j++){
                int clint_socket = get_connection(datamap.blocks[i].locations[j],datamap.blocks[i].ports[j]);
                send(clint_socket, msg, strlen(msg), 0);
                socket_addrs[n]=clint_socket;
                n++;
            }
        }
    }
    //send completed 
    return socket_addrs
}

void status_listen(int *ds_lists){
    for(int i=0; i<sizeof(ds_list)){

    }
}
