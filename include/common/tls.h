#ifndef DFS_TLS_H
#define DFS_TLS_H

#include <stddef.h>
#include <openssl/ssl.h>
#include "common/config.h"

int tls_library_init(void);
void tls_library_cleanup(void);

SSL_CTX *tls_create_server_ctx(const tls_settings_t *settings);
SSL_CTX *tls_create_client_ctx(const tls_settings_t *settings);

SSL *tls_accept_connection(SSL_CTX *ctx, int client_fd);
SSL *tls_wrap_client_socket(SSL_CTX *ctx, int sock_fd);
ssize_t tls_write_data(SSL *ssl, int fd, const void *buf, size_t len);
ssize_t tls_read_data(SSL *ssl, int fd, void *buf, size_t len);
void tls_close_connection(SSL *ssl);

#endif
