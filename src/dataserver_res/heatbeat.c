#include "dataserver/heatbeat.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "clint/connction_cash.h"

void *heardbeat(void *arg){
    int port = *((int*)arg);
    int metaserver = get_connection("127.0.0.1",port);
    char msg[100];
    sprintf(msg,"HEARTBEAT DS_ID:%d",port);
    while(1){
        send(metaserver,msg,strlen(msg),0);
        printf("\nsend the msg\n");
        sleep(5);
    }
    close(metaserver);
    return NULL;
}
