#include "common/tls.h"
#include "common/logging.h"
#include <openssl/err.h>
#include <sys/socket.h>
#include <unistd.h>

int tls_library_init(void) {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    return 0;
}

void tls_library_cleanup(void) {
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    EVP_cleanup();
    ERR_free_strings();
#else
    OPENSSL_cleanup();
#endif
}

static int configure_ctx_certificates(SSL_CTX *ctx, const tls_settings_t *settings, int is_server) {
    if (!settings) {
        return -1;
    }
    if (settings->cert_file[0] != '\0') {
        if (SSL_CTX_use_certificate_file(ctx, settings->cert_file, SSL_FILETYPE_PEM) <= 0) {
            LOG_ERROR("Failed to load certificate %s", settings->cert_file);
            return -1;
        }
    }
    if (settings->key_file[0] != '\0') {
        if (SSL_CTX_use_PrivateKey_file(ctx, settings->key_file, SSL_FILETYPE_PEM) <= 0) {
            LOG_ERROR("Failed to load private key %s", settings->key_file);
            return -1;
        }
        if (!SSL_CTX_check_private_key(ctx)) {
            LOG_ERROR("Private key does not match certificate");
            return -1;
        }
    }
    if (settings->ca_file[0] != '\0') {
        if (!SSL_CTX_load_verify_locations(ctx, settings->ca_file, NULL)) {
            LOG_ERROR("Failed to load CA file %s", settings->ca_file);
            return -1;
        }
    }

    if (settings->require_client_cert || (!is_server && settings->cert_file[0] != '\0')) {
        int mode = SSL_VERIFY_PEER;
        if (is_server && settings->require_client_cert) {
            mode |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
        }
        SSL_CTX_set_verify(ctx, mode, NULL);
        SSL_CTX_set_verify_depth(ctx, 4);
    }

    return 0;
}

SSL_CTX *tls_create_server_ctx(const tls_settings_t *settings) {
    if (!settings || !settings->enabled) {
        return NULL;
    }
    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        LOG_ERROR("Failed to create TLS server context");
        return NULL;
    }
    if (configure_ctx_certificates(ctx, settings, 1) != 0) {
        SSL_CTX_free(ctx);
        return NULL;
    }
    return ctx;
}

SSL_CTX *tls_create_client_ctx(const tls_settings_t *settings) {
    if (!settings || !settings->enabled) {
        return NULL;
    }
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        LOG_ERROR("Failed to create TLS client context");
        return NULL;
    }
    if (configure_ctx_certificates(ctx, settings, 0) != 0) {
        SSL_CTX_free(ctx);
        return NULL;
    }
    return ctx;
}

SSL *tls_accept_connection(SSL_CTX *ctx, int client_fd) {
    if (!ctx) {
        return NULL;
    }
    SSL *ssl = SSL_new(ctx);
    if (!ssl) {
        LOG_ERROR("Failed to allocate SSL object");
        return NULL;
    }
    SSL_set_fd(ssl, client_fd);
    if (SSL_accept(ssl) <= 0) {
        LOG_WARN("TLS accept handshake failed");
        SSL_free(ssl);
        return NULL;
    }
    return ssl;
}

SSL *tls_wrap_client_socket(SSL_CTX *ctx, int sock_fd) {
    if (!ctx) {
        return NULL;
    }
    SSL *ssl = SSL_new(ctx);
    if (!ssl) {
        LOG_ERROR("Failed to allocate SSL object");
        return NULL;
    }
    SSL_set_fd(ssl, sock_fd);
    if (SSL_connect(ssl) <= 0) {
        LOG_WARN("TLS connect handshake failed");
        SSL_free(ssl);
        return NULL;
    }
    return ssl;
}

ssize_t tls_write_data(SSL *ssl, int fd, const void *buf, size_t len) {
    if (ssl) {
        return SSL_write(ssl, buf, (int)len);
    }
    return send(fd, buf, len, 0);
}

ssize_t tls_read_data(SSL *ssl, int fd, void *buf, size_t len) {
    if (ssl) {
        return SSL_read(ssl, buf, (int)len);
    }
    return recv(fd, buf, len, 0);
}

void tls_close_connection(SSL *ssl) {
    if (!ssl) {
        return;
    }
    SSL_shutdown(ssl);
    SSL_free(ssl);
}
