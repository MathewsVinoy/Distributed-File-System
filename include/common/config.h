#ifndef DFS_CONFIG_H
#define DFS_CONFIG_H

#include <stddef.h>

#define DFS_ADDR_LEN 64
#define DFS_PATH_LEN 256

typedef struct {
    int enabled;
    int require_client_cert;
    char cert_file[DFS_PATH_LEN];
    char key_file[DFS_PATH_LEN];
    char ca_file[DFS_PATH_LEN];
} tls_settings_t;

typedef struct {
    char metadata_file[DFS_PATH_LEN];
    char last_seen_file[DFS_PATH_LEN];
    size_t block_size;

    struct {
        char listen_addr[DFS_ADDR_LEN];
        int listen_port;
        int heartbeat_port;
        tls_settings_t tls;
    } metadata;

    struct {
        char bind_addr[DFS_ADDR_LEN];
        int port;
        char data_file[DFS_PATH_LEN];
        char log_dir[DFS_PATH_LEN];
        char metadata_host[DFS_ADDR_LEN];
        int metadata_port;
        char heartbeat_host[DFS_ADDR_LEN];
        int heartbeat_port;
        tls_settings_t tls;
        tls_settings_t metadata_tls;
    } data;

    struct {
        char metadata_host[DFS_ADDR_LEN];
        int metadata_port;
        char output_file[DFS_PATH_LEN];
        tls_settings_t tls;
    } client;
} dfs_config_t;

int dfs_config_load(const char *path, dfs_config_t *config);
const char *dfs_config_get_error(void);

#endif
