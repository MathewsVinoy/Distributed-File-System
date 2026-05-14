#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "clint/lookup.h"
#include "clint/read_file.h"
#include "clint/writefuncs.h"
#include "common/config.h"
#include "common/logging.h"

#define HTTP_BUFFER_SIZE 65536
#define DEFAULT_LISTEN_ADDR "127.0.0.1"
#define DEFAULT_LISTEN_PORT 8080
#define DEFAULT_WEB_ROOT "webclient"
#define DEFAULT_USERS_FILE "config/users.csv"
#define MAX_USERS 64
#define MAX_SESSIONS 128
#define TOKEN_LEN 32
#define MAX_FILE_PREVIEW 65536

typedef struct {
    char username[64];
    char password[128];
    char root_path[PATH_MAX];
} user_record_t;

typedef struct {
    char token[TOKEN_LEN];
    const user_record_t *user;
    time_t issued_at;
} session_record_t;

static user_record_t g_users[MAX_USERS];
static size_t g_user_count = 0;
static session_record_t g_sessions[MAX_SESSIONS];

static int mkdir_recursive(const char *path);
static int recursive_delete(const char *path);

static void send_http_response(int client_fd, int status, const char *status_text, const char *body) {
    if (!body) {
        body = "{}";
    }
    char header[512];
    size_t body_len = strlen(body);
    int header_len = snprintf(header,
                              sizeof(header),
                              "HTTP/1.1 %d %s\r\n"
                              "Content-Type: application/json\r\n"
                              "Content-Length: %zu\r\n"
                              "Access-Control-Allow-Origin: *\r\n"
                              "Access-Control-Allow-Headers: Content-Type\r\n"
                              "Access-Control-Allow-Methods: POST, OPTIONS\r\n"
                              "Connection: close\r\n\r\n",
                              status,
                              status_text,
                              body_len);
    send(client_fd, header, (size_t)header_len, 0);
    send(client_fd, body, body_len, 0);
}

static void send_options_response(int client_fd) {
    const char *body = "{}";
    char header[512];
    int header_len = snprintf(header,
                              sizeof(header),
                              "HTTP/1.1 204 No Content\r\n"
                              "Access-Control-Allow-Origin: *\r\n"
                              "Access-Control-Allow-Headers: Content-Type\r\n"
                              "Access-Control-Allow-Methods: POST, OPTIONS\r\n"
                              "Content-Length: %zu\r\n"
                              "Connection: close\r\n\r\n",
                              strlen(body));
    send(client_fd, header, (size_t)header_len, 0);
    send(client_fd, body, strlen(body), 0);
}

static int parse_request_line(const char *buffer, char *method, size_t method_len, char *path, size_t path_len) {
    const char *space = strchr(buffer, ' ');
    if (!space) {
        return -1;
    }
    size_t mlen = (size_t)(space - buffer);
    if (mlen >= method_len) {
        return -1;
    }
    strncpy(method, buffer, mlen);
    method[mlen] = '\0';

    const char *path_start = space + 1;
    const char *path_end = strchr(path_start, ' ');
    if (!path_end) {
        return -1;
    }
    size_t plen = (size_t)(path_end - path_start);
    if (plen >= path_len) {
        return -1;
    }
    strncpy(path, path_start, plen);
    path[plen] = '\0';
    return 0;
}

static ssize_t read_http_request(int client_fd, char *buffer, size_t buffer_len, size_t *body_offset, size_t *content_length, char *method, size_t method_len, char *path, size_t path_len) {
    ssize_t total = 0;
    ssize_t n;
    int header_complete = 0;

    while ((size_t)total < buffer_len && (n = recv(client_fd, buffer + total, buffer_len - (size_t)total, 0)) > 0) {
        total += n;
        const char *marker = strstr(buffer, "\r\n\r\n");
        if (marker) {
            header_complete = 1;
            *body_offset = (size_t)(marker - buffer) + 4;
            break;
        }
    }

    if (n < 0) {
        return -1;
    }
    if (!header_complete) {
        return -1;
    }

    buffer[total] = '\0';
    if (parse_request_line(buffer, method, method_len, path, path_len) != 0) {
        return -1;
    }

    const char *length_header = strcasestr(buffer, "Content-Length:");
    if (length_header) {
        *content_length = (size_t)strtoul(length_header + 15, NULL, 10);
    } else {
        *content_length = 0;
    }

    if (*body_offset + *content_length >= buffer_len) {
        return -1;
    }

    while ((size_t)total < *body_offset + *content_length) {
        n = recv(client_fd, buffer + total, buffer_len - (size_t)total, 0);
        if (n <= 0) {
            return -1;
        }
        total += n;
    }
    buffer[total] = '\0';
    return total;
}

static const char *find_key(const char *json, const char *key) {
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    return strstr(json, pattern);
}

static int json_get_string(const char *json, const char *key, char *out, size_t out_len) {
    if (!json || !key || !out || out_len == 0) {
        return -1;
    }
    const char *match = find_key(json, key);
    if (!match) {
        return -1;
    }
    const char *colon = strchr(match, ':');
    if (!colon) {
        return -1;
    }
    const char *cursor = colon + 1;
    while (*cursor && isspace((unsigned char)*cursor)) {
        cursor++;
    }
    if (*cursor != '"') {
        return -1;
    }
    cursor++;
    size_t idx = 0;
    while (*cursor && *cursor != '"' && idx + 1 < out_len) {
        if (*cursor == '\\' && cursor[1]) {
            cursor++;
            char escaped = *cursor++;
            switch (escaped) {
                case 'n':
                    out[idx++] = '\n';
                    break;
                case 't':
                    out[idx++] = '\t';
                    break;
                case 'r':
                    out[idx++] = '\r';
                    break;
                case '\\':
                    out[idx++] = '\\';
                    break;
                case '"':
                    out[idx++] = '"';
                    break;
                default:
                    out[idx++] = escaped;
                    break;
            }
            continue;
        }
        out[idx++] = *cursor++;
    }
    out[idx] = '\0';
    return 0;
}

static int json_get_int(const char *json, const char *key, int *value_out) {
    const char *match = find_key(json, key);
    if (!match) {
        return -1;
    }
    const char *colon = strchr(match, ':');
    if (!colon) {
        return -1;
    }
    const char *cursor = colon + 1;
    while (*cursor && isspace((unsigned char)*cursor)) {
        cursor++;
    }
    char *endptr = NULL;
    long val = strtol(cursor, &endptr, 10);
    if (cursor == endptr) {
        return -1;
    }
    *value_out = (int)val;
    return 0;
}

static void rstrip(char *value) {
    if (!value) {
        return;
    }
    size_t len = strlen(value);
    while (len > 0 && isspace((unsigned char)value[len - 1])) {
        value[--len] = '\0';
    }
}

static int load_user_table(const char *users_file) {
    FILE *fp = fopen(users_file, "r");
    if (!fp) {
        return -1;
    }

    char line[512];
    size_t loaded = 0;
    while (fgets(line, sizeof(line), fp) && loaded < MAX_USERS) {
        if (line[0] == '#' || line[0] == '\0' || line[0] == '\n') {
            continue;
        }

        rstrip(line);
        char *username = strtok(line, ",");
        char *password = strtok(NULL, ",");
        char *root = strtok(NULL, ",");
        if (!username || !password || !root) {
            continue;
        }

        rstrip(username);
        rstrip(password);
        rstrip(root);

        snprintf(g_users[loaded].username, sizeof(g_users[loaded].username), "%s", username);
        snprintf(g_users[loaded].password, sizeof(g_users[loaded].password), "%s", password);
        snprintf(g_users[loaded].root_path, sizeof(g_users[loaded].root_path), "%s", root);
        mkdir_recursive(g_users[loaded].root_path);
        loaded++;
    }

    fclose(fp);
    g_user_count = loaded;
    return (int)loaded;
}

static void seed_sessions(void) {
    memset(g_sessions, 0, sizeof(g_sessions));
}

static void generate_token(char *out, size_t len) {
    static const char alphabet[] = "0123456789abcdef";
    if (len == 0) {
        return;
    }
    for (size_t i = 0; i + 1 < len; i++) {
        out[i] = alphabet[rand() % (sizeof(alphabet) - 1)];
    }
    out[len - 1] = '\0';
}

static const user_record_t *authenticate_user(const char *username, const char *password) {
    for (size_t i = 0; i < g_user_count; i++) {
        if (strcmp(g_users[i].username, username) == 0 && strcmp(g_users[i].password, password) == 0) {
            return &g_users[i];
        }
    }
    return NULL;
}

static session_record_t *reserve_session_slot(void) {
    session_record_t *oldest = &g_sessions[0];
    for (size_t i = 0; i < MAX_SESSIONS; i++) {
        if (g_sessions[i].token[0] == '\0') {
            return &g_sessions[i];
        }
        if (g_sessions[i].issued_at < oldest->issued_at) {
            oldest = &g_sessions[i];
        }
    }
    return oldest;
}

static const user_record_t *user_from_token(const char *token) {
    if (!token || token[0] == '\0') {
        return NULL;
    }
    for (size_t i = 0; i < MAX_SESSIONS; i++) {
        if (strcmp(g_sessions[i].token, token) == 0) {
            return g_sessions[i].user;
        }
    }
    return NULL;
}

static session_record_t *create_session(const user_record_t *user) {
    session_record_t *slot = reserve_session_slot();
    if (!slot) {
        return NULL;
    }
    generate_token(slot->token, sizeof(slot->token));
    slot->user = user;
    slot->issued_at = time(NULL);
    return slot;
}

static int resolve_user_path(const user_record_t *user, const char *request_path, char *out, size_t out_len) {
    if (!user || !out || out_len == 0) {
        return -1;
    }

    const char *relative = request_path && request_path[0] ? request_path : "/";
    if (strstr(relative, "..")) {
        return -1;
    }

    while (*relative == '/') {
        relative++;
    }

    if (*relative) {
        if (snprintf(out, out_len, "%s/%s", user->root_path, relative) >= (int)out_len) {
            return -1;
        }
    } else {
        if (snprintf(out, out_len, "%s", user->root_path) >= (int)out_len) {
            return -1;
        }
    }
    return 0;
}

static void json_escape(const char *input, char *output, size_t out_len) {
    if (!input || !output || out_len == 0) {
        return;
    }
    size_t idx = 0;
    while (*input && idx + 2 < out_len) {
        unsigned char c = (unsigned char)*input++;
        if (c == '"' || c == '\\') {
            output[idx++] = '\\';
            output[idx++] = (char)c;
        } else if (c == '\n') {
            output[idx++] = '\\';
            output[idx++] = 'n';
        } else if (c == '\r') {
            output[idx++] = '\\';
            output[idx++] = 'r';
        } else if (c == '\t') {
            output[idx++] = '\\';
            output[idx++] = 't';
        } else {
            output[idx++] = (char)c;
        }
    }
    output[idx] = '\0';
}

static int mkdir_recursive(const char *path) {
    char temp[PATH_MAX];
    snprintf(temp, sizeof(temp), "%s", path);
    size_t len = strlen(temp);
    if (len == 0) {
        return -1;
    }
    if (temp[len - 1] == '/') {
        temp[len - 1] = '\0';
    }

    for (char *p = temp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(temp, 0775);
            *p = '/';
        }
    }
    if (mkdir(temp, 0775) < 0 && errno != EEXIST) {
        return -1;
    }
    return 0;
}

static int recursive_delete(const char *path) {
    struct stat st;
    if (lstat(path, &st) < 0) {
        return -1;
    }

    if (S_ISDIR(st.st_mode)) {
        DIR *dir = opendir(path);
        if (!dir) {
            return -1;
        }
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            char child[PATH_MAX];
            if (snprintf(child, sizeof(child), "%s/%s", path, entry->d_name) >= (int)sizeof(child)) {
                closedir(dir);
                return -1;
            }
            if (recursive_delete(child) != 0) {
                closedir(dir);
                return -1;
            }
        }
        closedir(dir);
        if (rmdir(path) < 0) {
            return -1;
        }
    } else {
        if (unlink(path) < 0) {
            return -1;
        }
    }
    return 0;
}

static int read_file_contents(const char *path, char *buffer, size_t max_len, size_t *out_len) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        return -1;
    }
    size_t total = fread(buffer, 1, max_len - 1, fp);
    buffer[total] = '\0';
    fclose(fp);
    if (out_len) {
        *out_len = total;
    }
    return 0;
}

static int write_file_contents(const char *path, const char *payload, size_t payload_len) {
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        return -1;
    }
    size_t written = fwrite(payload, 1, payload_len, fp);
    fclose(fp);
    return written == payload_len ? 0 : -1;
}

static void handle_lookup_request(int client_fd, const dfs_config_t *config, const char *body) {
    char filename[MAX_FILENAME_LEN];
    int block_id;
    if (json_get_string(body, "filename", filename, sizeof(filename)) != 0 ||
        json_get_int(body, "block", &block_id) != 0) {
        send_http_response(client_fd, 400, "Bad Request", "{\"status\":\"error\",\"message\":\"Missing filename or block\"}");
        return;
    }

    blockinfo location;
    if (lookup_block_execute(config, filename, block_id, &location) != 0) {
        send_http_response(client_fd, 502, "Bad Gateway", "{\"status\":\"error\",\"message\":\"Metadata lookup failed\"}");
        return;
    }

    char replicas[1024];
    size_t offset = 0;
    offset += snprintf(replicas + offset, sizeof(replicas) - offset, "[");
    int first = 1;
    for (size_t i = 0; i < MAX_DS; i++) {
        if (location.ports[i] == 0 || location.locations[i][0] == '\0') {
            continue;
        }
        offset += snprintf(replicas + offset,
                           sizeof(replicas) - offset,
                           "%s{\"host\":\"%s\",\"port\":%d}",
                           first ? "" : ",",
                           location.locations[i],
                           location.ports[i]);
        first = 0;
    }
    snprintf(replicas + offset, sizeof(replicas) - offset, "]");

    char response[1400];
    snprintf(response,
             sizeof(response),
             "{\"status\":\"ok\",\"filename\":\"%s\",\"block\":%d,\"replicas\":%s}",
             filename,
             block_id,
             replicas);
    send_http_response(client_fd, 200, "OK", response);
}

static void handle_write_request(int client_fd, const dfs_config_t *config, const char *body) {
    char filename[MAX_FILENAME_LEN];
    int block_id;
    char payload[HTTP_BUFFER_SIZE / 2];
    if (json_get_string(body, "filename", filename, sizeof(filename)) != 0 ||
        json_get_int(body, "block", &block_id) != 0 ||
        json_get_string(body, "payload", payload, sizeof(payload)) != 0) {
        send_http_response(client_fd, 400, "Bad Request", "{\"status\":\"error\",\"message\":\"Missing filename, block, or payload\"}");
        return;
    }

    size_t payload_len = strlen(payload);
    if (payload_len == 0) {
        send_http_response(client_fd, 400, "Bad Request", "{\"status\":\"error\",\"message\":\"Payload empty\"}");
        return;
    }

    if (write_block_execute(config, filename, block_id, payload, payload_len) != 0) {
        send_http_response(client_fd, 502, "Bad Gateway", "{\"status\":\"error\",\"message\":\"Replica write failed\"}");
        return;
    }

    send_http_response(client_fd, 200, "OK", "{\"status\":\"ok\",\"message\":\"Write scheduled\"}");
}

static void handle_read_request(int client_fd, const dfs_config_t *config, const char *body) {
    char filename[MAX_FILENAME_LEN];
    char output_path[DFS_PATH_LEN] = {0};
    int has_output = json_get_string(body, "output", output_path, sizeof(output_path));
    if (json_get_string(body, "filename", filename, sizeof(filename)) != 0) {
        send_http_response(client_fd, 400, "Bad Request", "{\"status\":\"error\",\"message\":\"Missing filename\"}");
        return;
    }

    const char *target = has_output == 0 && output_path[0] ? output_path : config->client.output_file;

    if (read_file_execute(config, filename, target) != 0) {
        send_http_response(client_fd, 502, "Bad Gateway", "{\"status\":\"error\",\"message\":\"Read failed\"}");
        return;
    }

    char response[384];
    snprintf(response,
             sizeof(response),
             "{\"status\":\"ok\",\"filename\":\"%s\",\"output\":\"%s\"}",
             filename,
             target);
    send_http_response(client_fd, 200, "OK", response);
}

static void handle_login_request(int client_fd, const char *body) {
    char username[64];
    char password[128];
    if (json_get_string(body, "username", username, sizeof(username)) != 0 ||
        json_get_string(body, "password", password, sizeof(password)) != 0) {
        send_http_response(client_fd, 400, "Bad Request", "{\"status\":\"error\",\"message\":\"Missing credentials\"}");
        return;
    }

    const user_record_t *user = authenticate_user(username, password);
    if (!user) {
        send_http_response(client_fd, 401, "Unauthorized", "{\"status\":\"error\",\"message\":\"Invalid credentials\"}");
        return;
    }

    session_record_t *session = create_session(user);
    if (!session) {
        send_http_response(client_fd, 500, "Internal Server Error", "{\"status\":\"error\",\"message\":\"Session pool full\"}");
        return;
    }

    char response[256];
    snprintf(response,
             sizeof(response),
             "{\"status\":\"ok\",\"token\":\"%s\",\"home\":\"/\",\"user\":\"%s\"}",
             session->token,
             user->username);
    send_http_response(client_fd, 200, "OK", response);
}

static const user_record_t *authorize_request(int client_fd, const char *body, char *token_out, size_t token_len) {
    if (json_get_string(body, "token", token_out, token_len) != 0) {
        send_http_response(client_fd, 401, "Unauthorized", "{\"status\":\"error\",\"message\":\"Missing token\"}");
        return NULL;
    }
    const user_record_t *user = user_from_token(token_out);
    if (!user) {
        send_http_response(client_fd, 401, "Unauthorized", "{\"status\":\"error\",\"message\":\"Invalid token\"}");
        return NULL;
    }
    return user;
}

static void handle_fs_list(int client_fd, const char *body) {
    char token[TOKEN_LEN];
    const user_record_t *user = authorize_request(client_fd, body, token, sizeof(token));
    if (!user) {
        return;
    }

    char rel_path[PATH_MAX] = {'\0'};
    if (json_get_string(body, "path", rel_path, sizeof(rel_path)) != 0) {
        rel_path[0] = '\0';
    }

    char abs_path[PATH_MAX];
    if (resolve_user_path(user, rel_path, abs_path, sizeof(abs_path)) != 0) {
        send_http_response(client_fd, 400, "Bad Request", "{\"status\":\"error\",\"message\":\"Invalid path\"}");
        return;
    }

    DIR *dir = opendir(abs_path);
    if (!dir) {
        send_http_response(client_fd, 404, "Not Found", "{\"status\":\"error\",\"message\":\"Directory missing\"}");
        return;
    }

    char response[HTTP_BUFFER_SIZE];
    const char *echo_path = (rel_path[0] == '\0') ? "/" : rel_path;
    size_t offset = snprintf(response,
                             sizeof(response),
                             "{\"status\":\"ok\",\"path\":\"%s\",\"entries\":[",
                             echo_path);
    struct dirent *entry;
    int first = 1;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        char entry_path[PATH_MAX];
        if (snprintf(entry_path, sizeof(entry_path), "%s/%s", abs_path, entry->d_name) >= (int)sizeof(entry_path)) {
            continue;
        }
        struct stat st;
        if (lstat(entry_path, &st) != 0) {
            continue;
        }
        char escaped[512];
        json_escape(entry->d_name, escaped, sizeof(escaped));
        const char *type = S_ISDIR(st.st_mode) ? "dir" : "file";
        offset += snprintf(response + offset,
                           sizeof(response) - offset,
                           "%s{\"name\":\"%s\",\"type\":\"%s\",\"size\":%lld,\"modified\":%lld}",
                           first ? "" : ",",
                           escaped,
                           type,
                           (long long)st.st_size,
                           (long long)st.st_mtime);
        if (offset >= sizeof(response) - 128) {
            break;
        }
        first = 0;
    }
    closedir(dir);
    snprintf(response + offset, sizeof(response) - offset, "]}");
    send_http_response(client_fd, 200, "OK", response);
}

static void handle_fs_mkdir(int client_fd, const char *body) {
    char token[TOKEN_LEN];
    const user_record_t *user = authorize_request(client_fd, body, token, sizeof(token));
    if (!user) {
        return;
    }

    char rel_path[PATH_MAX];
    if (json_get_string(body, "path", rel_path, sizeof(rel_path)) != 0 || rel_path[0] == '\0') {
        send_http_response(client_fd, 400, "Bad Request", "{\"status\":\"error\",\"message\":\"Path required\"}");
        return;
    }

    char abs_path[PATH_MAX];
    if (resolve_user_path(user, rel_path, abs_path, sizeof(abs_path)) != 0) {
        send_http_response(client_fd, 400, "Bad Request", "{\"status\":\"error\",\"message\":\"Invalid path\"}");
        return;
    }

    if (mkdir_recursive(abs_path) != 0) {
        send_http_response(client_fd, 500, "Internal Server Error", "{\"status\":\"error\",\"message\":\"mkdir failed\"}");
        return;
    }

    send_http_response(client_fd, 200, "OK", "{\"status\":\"ok\"}");
}

static void handle_fs_delete(int client_fd, const char *body) {
    char token[TOKEN_LEN];
    const user_record_t *user = authorize_request(client_fd, body, token, sizeof(token));
    if (!user) {
        return;
    }

    char rel_path[PATH_MAX];
    if (json_get_string(body, "path", rel_path, sizeof(rel_path)) != 0 || rel_path[0] == '\0') {
        send_http_response(client_fd, 400, "Bad Request", "{\"status\":\"error\",\"message\":\"Path required\"}");
        return;
    }

    char abs_path[PATH_MAX];
    if (resolve_user_path(user, rel_path, abs_path, sizeof(abs_path)) != 0) {
        send_http_response(client_fd, 400, "Bad Request", "{\"status\":\"error\",\"message\":\"Invalid path\"}");
        return;
    }

    if (recursive_delete(abs_path) != 0) {
        send_http_response(client_fd, 500, "Internal Server Error", "{\"status\":\"error\",\"message\":\"Delete failed\"}");
        return;
    }

    send_http_response(client_fd, 200, "OK", "{\"status\":\"ok\"}");
}

static void handle_fs_write(int client_fd, const char *body) {
    char token[TOKEN_LEN];
    const user_record_t *user = authorize_request(client_fd, body, token, sizeof(token));
    if (!user) {
        return;
    }

    char rel_path[PATH_MAX];
    char content[MAX_FILE_PREVIEW];
    if (json_get_string(body, "path", rel_path, sizeof(rel_path)) != 0 || rel_path[0] == '\0' ||
        json_get_string(body, "content", content, sizeof(content)) != 0) {
        send_http_response(client_fd, 400, "Bad Request", "{\"status\":\"error\",\"message\":\"Path and content required\"}");
        return;
    }

    char abs_path[PATH_MAX];
    if (resolve_user_path(user, rel_path, abs_path, sizeof(abs_path)) != 0) {
        send_http_response(client_fd, 400, "Bad Request", "{\"status\":\"error\",\"message\":\"Invalid path\"}");
        return;
    }

    if (write_file_contents(abs_path, content, strlen(content)) != 0) {
        send_http_response(client_fd, 500, "Internal Server Error", "{\"status\":\"error\",\"message\":\"Write failed\"}");
        return;
    }

    send_http_response(client_fd, 200, "OK", "{\"status\":\"ok\"}");
}

static void handle_fs_read_file(int client_fd, const char *body) {
    char token[TOKEN_LEN];
    const user_record_t *user = authorize_request(client_fd, body, token, sizeof(token));
    if (!user) {
        return;
    }

    char rel_path[PATH_MAX];
    if (json_get_string(body, "path", rel_path, sizeof(rel_path)) != 0 || rel_path[0] == '\0') {
        send_http_response(client_fd, 400, "Bad Request", "{\"status\":\"error\",\"message\":\"Path required\"}");
        return;
    }

    char abs_path[PATH_MAX];
    if (resolve_user_path(user, rel_path, abs_path, sizeof(abs_path)) != 0) {
        send_http_response(client_fd, 400, "Bad Request", "{\"status\":\"error\",\"message\":\"Invalid path\"}");
        return;
    }

    char buffer[MAX_FILE_PREVIEW];
    size_t read_len = 0;
    if (read_file_contents(abs_path, buffer, sizeof(buffer), &read_len) != 0) {
        send_http_response(client_fd, 404, "Not Found", "{\"status\":\"error\",\"message\":\"Read failed\"}");
        return;
    }

    char escaped[MAX_FILE_PREVIEW * 2];
    json_escape(buffer, escaped, sizeof(escaped));

    size_t escaped_len = strlen(escaped);
    size_t rel_len = strlen(rel_path);
    size_t needed = escaped_len + rel_len + 128;
    char *response = malloc(needed);
    if (!response) {
        send_http_response(client_fd, 500, "Internal Server Error", "{\"status\":\"error\",\"message\":\"OOM\"}");
        return;
    }

    int written = snprintf(response,
                           needed,
                           "{\"status\":\"ok\",\"path\":\"%s\",\"content\":\"%s\",\"bytes\":%zu}",
                           rel_path,
                           escaped,
                           read_len);
    if (written < 0 || (size_t)written >= needed) {
        free(response);
        send_http_response(client_fd, 413, "Payload Too Large", "{\"status\":\"error\",\"message\":\"Response too large\"}");
        return;
    }

    send_http_response(client_fd, 200, "OK", response);
    free(response);
}

static const char *mime_type_from_path(const char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot || dot == path) {
        return "text/plain; charset=utf-8";
    }
    dot++;

    if (strcasecmp(dot, "html") == 0) {
        return "text/html; charset=utf-8";
    }
    if (strcasecmp(dot, "css") == 0) {
        return "text/css; charset=utf-8";
    }
    if (strcasecmp(dot, "js") == 0) {
        return "application/javascript; charset=utf-8";
    }
    if (strcasecmp(dot, "json") == 0) {
        return "application/json; charset=utf-8";
    }
    if (strcasecmp(dot, "svg") == 0) {
        return "image/svg+xml";
    }
    if (strcasecmp(dot, "png") == 0) {
        return "image/png";
    }
    if (strcasecmp(dot, "jpg") == 0 || strcasecmp(dot, "jpeg") == 0) {
        return "image/jpeg";
    }

    return "application/octet-stream";
}

static int send_static_file(int client_fd, const char *web_root, const char *request_path) {
    if (!web_root || !request_path) {
        send_http_response(client_fd, 500, "Internal Server Error", "{\"status\":\"error\",\"message\":\"Static handler misconfigured\"}");
        return -1;
    }

    if (strstr(request_path, "..")) {
        send_http_response(client_fd, 400, "Bad Request", "{\"status\":\"error\",\"message\":\"Invalid path\"}");
        return -1;
    }

    const char *relative = (*request_path == '\0') ? "/" : request_path;
    if (strcmp(relative, "/") == 0) {
        relative = "/index.html";
    }

    char full_path[PATH_MAX];
    if (snprintf(full_path, sizeof(full_path), "%s%s", web_root, relative) >= (int)sizeof(full_path)) {
        send_http_response(client_fd, 414, "URI Too Long", "{\"status\":\"error\",\"message\":\"Path too long\"}");
        return -1;
    }

    int fd = open(full_path, O_RDONLY);
    if (fd < 0) {
        send_http_response(client_fd, 404, "Not Found", "{\"status\":\"error\",\"message\":\"File missing\"}");
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0 || !S_ISREG(st.st_mode)) {
        close(fd);
        send_http_response(client_fd, 404, "Not Found", "{\"status\":\"error\",\"message\":\"File missing\"}");
        return -1;
    }

    size_t file_len = (size_t)st.st_size;
    const char *mime = mime_type_from_path(relative);

    char header[512];
    int header_len = snprintf(header,
                              sizeof(header),
                              "HTTP/1.1 200 OK\r\n"
                              "Content-Type: %s\r\n"
                              "Content-Length: %zu\r\n"
                              "Cache-Control: no-cache\r\n"
                              "Connection: close\r\n\r\n",
                              mime,
                              file_len);
    send(client_fd, header, (size_t)header_len, 0);

    char buffer[4096];
    ssize_t read_bytes;
    while ((read_bytes = read(fd, buffer, sizeof(buffer))) > 0) {
        send(client_fd, buffer, (size_t)read_bytes, 0);
    }

    close(fd);
    return 0;
}

static void dispatch_request(int client_fd, const dfs_config_t *config, const char *method, const char *path, const char *body, const char *web_root) {
    if (strcmp(method, "OPTIONS") == 0) {
        send_options_response(client_fd);
        return;
    }

    if (strcmp(method, "GET") == 0) {
        if (strcmp(path, "/healthz") == 0) {
            send_http_response(client_fd, 200, "OK", "{\"status\":\"ok\"}");
        } else {
            send_static_file(client_fd, web_root, path);
        }
        return;
    }

    if (strcmp(method, "POST") != 0) {
        send_http_response(client_fd, 405, "Method Not Allowed", "{\"status\":\"error\",\"message\":\"Only POST supported\"}");
        return;
    }

    if (strcmp(path, "/auth/login") == 0) {
        handle_login_request(client_fd, body);
    } else if (strcmp(path, "/fs/list") == 0) {
        handle_fs_list(client_fd, body);
    } else if (strcmp(path, "/fs/mkdir") == 0) {
        handle_fs_mkdir(client_fd, body);
    } else if (strcmp(path, "/fs/delete") == 0) {
        handle_fs_delete(client_fd, body);
    } else if (strcmp(path, "/fs/write") == 0) {
        handle_fs_write(client_fd, body);
    } else if (strcmp(path, "/fs/read") == 0) {
        handle_fs_read_file(client_fd, body);
    } else if (strcmp(path, "/client/lookup") == 0) {
        handle_lookup_request(client_fd, config, body);
    } else if (strcmp(path, "/client/write") == 0) {
        handle_write_request(client_fd, config, body);
    } else if (strcmp(path, "/client/read") == 0) {
        handle_read_request(client_fd, config, body);
    } else {
        send_http_response(client_fd, 404, "Not Found", "{\"status\":\"error\",\"message\":\"Unknown endpoint\"}");
    }
}

static int create_listen_socket(const char *addr, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(addr);

    if (bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }

    if (listen(fd, 16) < 0) {
        perror("listen");
        close(fd);
        return -1;
    }

    return fd;
}

int main(int argc, char *argv[]) {
    const char *config_path = (argc > 1) ? argv[1] : "config/dfs.conf";
    const char *listen_addr = (argc > 2) ? argv[2] : DEFAULT_LISTEN_ADDR;
    int listen_port = (argc > 3) ? atoi(argv[3]) : DEFAULT_LISTEN_PORT;
    const char *web_root = (argc > 4) ? argv[4] : DEFAULT_WEB_ROOT;
    const char *users_file = (argc > 5) ? argv[5] : DEFAULT_USERS_FILE;

    dfs_config_t config;
    if (dfs_config_load(config_path, &config) != 0) {
        fprintf(stderr, "Failed to load config: %s\n", dfs_config_get_error());
        return EXIT_FAILURE;
    }

    if (load_user_table(users_file) <= 0) {
        fprintf(stderr, "Failed to load users from %s\n", users_file);
        return EXIT_FAILURE;
    }

    srand((unsigned)time(NULL));
    seed_sessions();

    log_init(stdout);
    log_set_level(LOG_LEVEL_INFO);
    signal(SIGPIPE, SIG_IGN);

    int server_fd = create_listen_socket(listen_addr, listen_port);
    if (server_fd < 0) {
        log_shutdown();
        return EXIT_FAILURE;
    }

    LOG_INFO("HTTP client bridge listening on %s:%d (web root: %s, users: %zu)", listen_addr, listen_port, web_root, g_user_count);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        char buffer[HTTP_BUFFER_SIZE + 1];
        size_t body_offset = 0;
        size_t content_length = 0;
        char method[8];
        char path[128];
        ssize_t bytes = read_http_request(client_fd,
                                          buffer,
                                          sizeof(buffer) - 1,
                                          &body_offset,
                                          &content_length,
                                          method,
                                          sizeof(method),
                                          path,
                                          sizeof(path));
        if (bytes < 0) {
            send_http_response(client_fd, 400, "Bad Request", "{\"status\":\"error\",\"message\":\"Malformed request\"}");
            close(client_fd);
            continue;
        }

        const char *body = buffer + body_offset;
        dispatch_request(client_fd, &config, method, path, body, web_root);
        close(client_fd);
    }

    close(server_fd);
    log_shutdown();
    return 0;
}
