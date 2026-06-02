#include "common/config.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define DEFAULT_CONFIG_PATH "config/dfs.conf"

static char last_error[256];

static void set_error(const char *msg) {
    strncpy(last_error, msg, sizeof(last_error) - 1);
    last_error[sizeof(last_error) - 1] = '\0';
}

const char *dfs_config_get_error(void) {
    return last_error;
}

static void trim(char *str) {
    size_t len = strlen(str);
    size_t start = 0;
    while (start < len && isspace((unsigned char)str[start])) {
        start++;
    }
    size_t end = len;
    while (end > start && isspace((unsigned char)str[end - 1])) {
        end--;
    }
    if (start > 0 || end < len) {
        memmove(str, str + start, end - start);
    }
    str[end - start] = '\0';
}

static int parse_bool(const char *value) {
    if (!value) {
        return 0;
    }
    if (strcasecmp(value, "true") == 0 || strcmp(value, "1") == 0 || strcasecmp(value, "yes") == 0 || strcasecmp(value, "on") == 0) {
        return 1;
    }
    return 0;
}

static void tls_defaults(tls_settings_t *tls) {
    memset(tls, 0, sizeof(*tls));
}

static void set_defaults(dfs_config_t *config) {
    memset(config, 0, sizeof(*config));
    strncpy(config->metadata_file, "database/metadata.txt", DFS_PATH_LEN - 1);
    strncpy(config->last_seen_file, "database/lastseen.csv", DFS_PATH_LEN - 1);
    config->block_size = 1024;

    strncpy(config->metadata.listen_addr, "0.0.0.0", DFS_ADDR_LEN - 1);
    config->metadata.listen_port = 9000;
    config->metadata.heartbeat_port = 9001;
    tls_defaults(&config->metadata.tls);

    strncpy(config->data.bind_addr, "0.0.0.0", DFS_ADDR_LEN - 1);
    config->data.port = 8000;
    strncpy(config->data.data_file, "database/my_file.txt", DFS_PATH_LEN - 1);
    strncpy(config->data.log_dir, "database/log", DFS_PATH_LEN - 1);
    strncpy(config->data.metadata_host, "127.0.0.1", DFS_ADDR_LEN - 1);
    config->data.metadata_port = 9000;
    strncpy(config->data.heartbeat_host, "127.0.0.1", DFS_ADDR_LEN - 1);
    config->data.heartbeat_port = 9001;
    tls_defaults(&config->data.tls);
    tls_defaults(&config->data.metadata_tls);

    strncpy(config->client.metadata_host, "127.0.0.1", DFS_ADDR_LEN - 1);
    config->client.metadata_port = 9000;
    strncpy(config->client.output_file, "out/cli/myfile.txt", DFS_PATH_LEN - 1);
    tls_defaults(&config->client.tls);
}

static void assign_value(dfs_config_t *config, const char *section, const char *key, const char *value) {
    if (strcmp(section, "global") == 0) {
        if (strcmp(key, "block_size") == 0) {
            config->block_size = (size_t)strtoul(value, NULL, 10);
        } else if (strcmp(key, "metadata_file") == 0) {
            strncpy(config->metadata_file, value, DFS_PATH_LEN - 1);
        } else if (strcmp(key, "last_seen_file") == 0) {
            strncpy(config->last_seen_file, value, DFS_PATH_LEN - 1);
        }
    } else if (strcmp(section, "metadata") == 0) {
        if (strcmp(key, "listen_addr") == 0) {
            strncpy(config->metadata.listen_addr, value, DFS_ADDR_LEN - 1);
        } else if (strcmp(key, "listen_port") == 0) {
            config->metadata.listen_port = atoi(value);
        } else if (strcmp(key, "heartbeat_port") == 0) {
            config->metadata.heartbeat_port = atoi(value);
        } else if (strcmp(key, "tls_enabled") == 0) {
            config->metadata.tls.enabled = parse_bool(value);
        } else if (strcmp(key, "tls_cert_file") == 0) {
            strncpy(config->metadata.tls.cert_file, value, DFS_PATH_LEN - 1);
        } else if (strcmp(key, "tls_key_file") == 0) {
            strncpy(config->metadata.tls.key_file, value, DFS_PATH_LEN - 1);
        } else if (strcmp(key, "tls_ca_file") == 0) {
            strncpy(config->metadata.tls.ca_file, value, DFS_PATH_LEN - 1);
        } else if (strcmp(key, "tls_require_client_cert") == 0) {
            config->metadata.tls.require_client_cert = parse_bool(value);
        }
    } else if (strcmp(section, "data") == 0) {
        if (strcmp(key, "bind_addr") == 0) {
            strncpy(config->data.bind_addr, value, DFS_ADDR_LEN - 1);
        } else if (strcmp(key, "port") == 0) {
            config->data.port = atoi(value);
        } else if (strcmp(key, "data_file") == 0) {
            strncpy(config->data.data_file, value, DFS_PATH_LEN - 1);
        } else if (strcmp(key, "log_dir") == 0) {
            strncpy(config->data.log_dir, value, DFS_PATH_LEN - 1);
        } else if (strcmp(key, "metadata_host") == 0) {
            strncpy(config->data.metadata_host, value, DFS_ADDR_LEN - 1);
        } else if (strcmp(key, "metadata_port") == 0) {
            config->data.metadata_port = atoi(value);
        } else if (strcmp(key, "heartbeat_host") == 0) {
            strncpy(config->data.heartbeat_host, value, DFS_ADDR_LEN - 1);
        } else if (strcmp(key, "heartbeat_port") == 0) {
            config->data.heartbeat_port = atoi(value);
        } else if (strcmp(key, "tls_enabled") == 0) {
            config->data.tls.enabled = parse_bool(value);
        } else if (strcmp(key, "tls_cert_file") == 0) {
            strncpy(config->data.tls.cert_file, value, DFS_PATH_LEN - 1);
        } else if (strcmp(key, "tls_key_file") == 0) {
            strncpy(config->data.tls.key_file, value, DFS_PATH_LEN - 1);
        } else if (strcmp(key, "tls_ca_file") == 0) {
            strncpy(config->data.tls.ca_file, value, DFS_PATH_LEN - 1);
        } else if (strcmp(key, "tls_require_client_cert") == 0) {
            config->data.tls.require_client_cert = parse_bool(value);
        } else if (strcmp(key, "metadata_tls_enabled") == 0) {
            config->data.metadata_tls.enabled = parse_bool(value);
        } else if (strcmp(key, "metadata_tls_cert_file") == 0) {
            strncpy(config->data.metadata_tls.cert_file, value, DFS_PATH_LEN - 1);
        } else if (strcmp(key, "metadata_tls_key_file") == 0) {
            strncpy(config->data.metadata_tls.key_file, value, DFS_PATH_LEN - 1);
        } else if (strcmp(key, "metadata_tls_ca_file") == 0) {
            strncpy(config->data.metadata_tls.ca_file, value, DFS_PATH_LEN - 1);
        } else if (strcmp(key, "metadata_tls_require_client_cert") == 0) {
            config->data.metadata_tls.require_client_cert = parse_bool(value);
        }
    } else if (strcmp(section, "client") == 0) {
        if (strcmp(key, "metadata_host") == 0) {
            strncpy(config->client.metadata_host, value, DFS_ADDR_LEN - 1);
        } else if (strcmp(key, "metadata_port") == 0) {
            config->client.metadata_port = atoi(value);
        } else if (strcmp(key, "output_file") == 0) {
            strncpy(config->client.output_file, value, DFS_PATH_LEN - 1);
        } else if (strcmp(key, "tls_enabled") == 0) {
            config->client.tls.enabled = parse_bool(value);
        } else if (strcmp(key, "tls_cert_file") == 0) {
            strncpy(config->client.tls.cert_file, value, DFS_PATH_LEN - 1);
        } else if (strcmp(key, "tls_key_file") == 0) {
            strncpy(config->client.tls.key_file, value, DFS_PATH_LEN - 1);
        } else if (strcmp(key, "tls_ca_file") == 0) {
            strncpy(config->client.tls.ca_file, value, DFS_PATH_LEN - 1);
        }
    }
}

int dfs_config_load(const char *path, dfs_config_t *config) {
    if (!config) {
        set_error("config pointer is NULL");
        return -1;
    }

    const char *cfg_path = path ? path : DEFAULT_CONFIG_PATH;
    FILE *fp = fopen(cfg_path, "r");
    if (!fp) {
        set_error("failed to open config file");
        return -1;
    }

    set_defaults(config);

    char line[512];
    char section[32] = "global";

    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        if (line[0] == '\0' || line[0] == '#' || line[0] == ';') {
            continue;
        }
        if (line[0] == '[') {
            char *end = strchr(line, ']');
            if (end) {
                *end = '\0';
                strncpy(section, line + 1, sizeof(section) - 1);
                section[sizeof(section) - 1] = '\0';
            }
            continue;
        }

        char *equals = strchr(line, '=');
        if (!equals) {
            continue;
        }
        *equals = '\0';
        char *key = line;
        char *value = equals + 1;
        trim(key);
        trim(value);
        assign_value(config, section, key, value);
    }

    fclose(fp);
    return 0;
}
