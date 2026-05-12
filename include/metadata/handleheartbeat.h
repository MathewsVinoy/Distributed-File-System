#ifndef HANDLEHEARTBEAT_H
#define HANDLEHEARTBEAT_H

#include <pthread.h>
#include <netinet/in.h>
#include <openssl/ssl.h>

typedef struct {
	int server_fd;
	struct sockaddr_in *client_addr;
	socklen_t *addr_len;
	const char *last_seen_path;
	SSL_CTX *tls_ctx;
	int tls_enabled;
} heartbeat_thread_args_t;

/*
 * Handle incoming heartbeat messages from data servers
 * arg: pointer to server socket fd and client addressing info (passed as struct)
 * Returns: NULL on thread exit
 */
void *handleHeartBeat(void *arg);

/*
 * Update the last seen timestamp for a data server
 * buffer: heartbeat message in format "HEARTBEAT DS_ID <port>"
 */
void update_last_seen(const char *buffer, const char *last_seen_file);

#endif
