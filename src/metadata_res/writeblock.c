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

void writeblock (int server_fd, char filename[100]){
    FileMap full_map = findlocation(filename);
    int i;
    blockinfo block;
    for(i=0;i<full_map.total_blocks;i++){
        int clint_socket = get_connection(datamap.blocks[i].locations[j],datamap.blocks[i].ports[j]);
        char request[100];
        
        
        //complete the connection code later 
        //need to connect to all other servers and send an requsite that need to send data or  not
    }
}

