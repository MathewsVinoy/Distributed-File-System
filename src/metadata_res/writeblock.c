#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "load_metadata.h"
#include "datastruct.h"
#include "metadata/getlocation.h"


void writeblock (int server_fd){
    FileMap full_map = findlocation(filename);
    int i;
    blockinfo block;
    for(i=0;i<full_map.total_blocks;i++){
        block = fullmap.blocks[i];
        //complete the connection code later 
        //need to connect to all other servers and send an requsite that need to send data or  not
    }
}

