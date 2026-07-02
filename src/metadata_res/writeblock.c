#include "metadata/writeblock.h"
#include "common/logging.h"
#include "common/tls.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

static ssize_t ds_send(int sock, SSL *ssl, const void *buf, size_t len) {
    return tls_write_data(ssl, sock, buf, len);
}

static ssize_t ds_recv(int sock, SSL *ssl, void *buf, size_t len) {
    return tls_read_data(ssl, sock, buf, len);
}

static int connect_to_dataserver(const char *ip, int port, int use_tls, SSL_CTX *client_ctx, SSL **out_ssl) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        LOG_ERROR("Failed to create socket for %s:%d", ip, port);
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        LOG_ERROR("Invalid data server address %s", ip);
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG_WARN("Connection to data server %s:%d failed", ip, port);
        close(sock);
        return -1;
    }

    SSL *ssl = NULL;
    if (use_tls) {
        if (!client_ctx) {
            LOG_ERROR("TLS required for data server %s:%d but client context missing", ip, port);
            close(sock);
            return -1;
        }
        ssl = tls_wrap_client_socket(client_ctx, sock);
        if (!ssl) {
            LOG_WARN("TLS handshake failed for data server %s:%d", ip, port);
            close(sock);
            return -1;
        }
    }

    if (out_ssl) {
        *out_ssl = ssl;
    } else if (ssl) {
        tls_close_connection(ssl);
    }
    return sock;
}

int prepare_write_block(const FileMap *datamap, int blockid, write_session_t *session, const dfs_config_t *config, SSL_CTX *client_ctx) {
    if (!datamap || !session) {
        return -1;
    }
    memset(session, 0, sizeof(*session));
    session->use_tls = config && config->data.tls.enabled;

    char msg[64];
    snprintf(msg, sizeof(msg), "PREPARE_WRITE BLOCK %d", blockid);

    for (int i = 0; i < datamap->total_blocks; ++i) {
        if (datamap->blocks[i].blockid != blockid) {
            continue;
        }
        for (int j = 0; j < MAX_DS; ++j) {
            if (datamap->blocks[i].ports[j] == 0 || datamap->blocks[i].locations[j][0] == '\0') {
                continue;
            }
            SSL *ssl = NULL;
            int sock = connect_to_dataserver(datamap->blocks[i].locations[j], datamap->blocks[i].ports[j], session->use_tls, client_ctx, &ssl);
            if (sock < 0) {
                continue;
            }
            if (ds_send(sock, ssl, msg, strlen(msg)) <= 0) {
                LOG_WARN("Failed to send prepare message to %s:%d", datamap->blocks[i].locations[j], datamap->blocks[i].ports[j]);
                if (ssl) tls_close_connection(ssl);
                close(sock);
                continue;
            }
            char buffer[32] = {0};
            ssize_t n = ds_recv(sock, ssl, buffer, sizeof(buffer) - 1);
            if (n <= 0 || strncmp(buffer, "ok", 2) != 0) {
                LOG_WARN("Data server %s:%d rejected prepare", datamap->blocks[i].locations[j], datamap->blocks[i].ports[j]);
                session->abort_required = 1;
                if (ssl) tls_close_connection(ssl);
                close(sock);
                continue;
            }
            session->sockets[session->count] = sock;
            session->ssl_handles[session->count] = ssl;
            session->count++;
        }
        break;
    }

    if (session->count == 0) {
        LOG_ERROR("No data servers acknowledged prepare for block %d", blockid);
        return -1;
    }
    return 0;
}

void status_listen(write_session_t *session, int blockid) {
    if (!session) {
        return;
    }
    char reply[64];
    for (size_t i = 0; i < session->count; ++i) {
        if (session->abort_required) {
            snprintf(reply, sizeof(reply), "GLOBAL_ABORT BLOCK %d", blockid);
        } else {
            snprintf(reply, sizeof(reply), "GLOBAL_COMMIT BLOCK %d", blockid);
        }
        ds_send(session->sockets[i], session->ssl_handles[i], reply, strlen(reply));
        if (session->ssl_handles[i]) {
            tls_close_connection(session->ssl_handles[i]);
            session->ssl_handles[i] = NULL;
        }
        close(session->sockets[i]);
        session->sockets[i] = -1;
    }
    session->count = 0;
}
