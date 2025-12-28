#include "dataserver/heatbeat.h"
#include <stdio.h>
#include <unistd.h>

void *heardbeat(void *arg){
    while(1){
        printf("\n\n\t\t\tHeart Beat sented\n\n");
        sleep(5);
    }
    return NULL;
}
