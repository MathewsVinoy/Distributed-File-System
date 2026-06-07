#include "dataserver/heatbeat.h"
#include "common/logging.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>

static int heartbeat_connect(const dataserver_heartbeat_args_t *cfg) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        LOG_ERROR("Heartbeat: socket() failed - %s", strerror(errno));
        return -1;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(cfg->port);
    if (inet_pton(AF_INET, cfg->host, &addr.sin_addr) <= 0) {
        LOG_ERROR("Heartbeat: invalid host %s", cfg->host);
        close(sock);
        return -1;
    }
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    return sock;
}

void *heartbeat(void *arg){
    if (!arg) {
        LOG_ERROR("Heartbeat: Invalid argument");
        pthread_exit(NULL);
    }

    dataserver_heartbeat_args_t cfg;
    memcpy(&cfg, arg, sizeof(cfg));
    free(arg);

    char msg[128];
    snprintf(msg, sizeof(msg), "HEARTBEAT DS_ID %d", cfg.data_server_port);

    while (1) {
        int sock = heartbeat_connect(&cfg);
        if (sock >= 0) {
            if (send(sock, msg, strlen(msg), 0) < 0) {
                LOG_WARN("Heartbeat send failed: %s", strerror(errno));
            } else {
                LOG_DEBUG("Heartbeat sent for DS %d", cfg.data_server_port);
            }
            close(sock);
        } else {
            LOG_WARN("Heartbeat: unable to reach metadata server %s:%d", cfg.host, cfg.port);
        }
        sleep(5);
    }

    return NULL;
}
