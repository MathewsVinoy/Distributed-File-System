#ifndef HEATBEAT_H
#define HEATBEAT_H

#include <pthread.h>
#include "common/config.h"

typedef struct {
	char host[DFS_ADDR_LEN];
	int port;
	int data_server_port;
} dataserver_heartbeat_args_t;

void* heartbeat(void* arg);

#endif
