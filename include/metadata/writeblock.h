#ifndef WRITEBLOCK_H
#define WRITEBLOCK_H

#include <stddef.h>
#include <openssl/ssl.h>
#include "datastruct.h"
#include "common/config.h"

typedef struct {
	int sockets[MAX_DS];
	SSL *ssl_handles[MAX_DS];
	size_t count;
	int abort_required;
    int use_tls;
} write_session_t;

int prepare_write_block(const FileMap *datamap, int blockid, write_session_t *session, const dfs_config_t *config, SSL_CTX *client_ctx);
void status_listen(write_session_t *session, int blockid);

#endif
