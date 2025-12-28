#include "dataserver/heatbeat.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void *heardbeat(void *arg){
    int port = *((int*)arg);
    while(1){
        printf("\n\n\t\t\tHeart Beat sented = %d\n\n",port);
        sleep(5);
    }
    return NULL;
}
