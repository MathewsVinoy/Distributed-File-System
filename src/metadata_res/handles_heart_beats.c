#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include "metadata/handleheartbeat.h"
#include "common/logging.h"
#include "common/config.h"
#include "common/tls.h"

/*
 * Handle incoming heartbeat messages from data servers
 * Runs in a separate thread listening for heartbeats
 */
void *handleHeartBeat(void *arg){
    if (arg == NULL) {
        fprintf(stderr, "HeartBeat Handler: Invalid argument\n");
        pthread_exit(NULL);
    }

    heartbeat_thread_args_t *hb_args = (heartbeat_thread_args_t *)arg;
    int server_fd = hb_args->server_fd;
    struct sockaddr_in *client_addr = hb_args->client_addr;
    socklen_t *addr_len = hb_args->addr_len;
    const char *last_seen_path = hb_args->last_seen_path;

    char buffer[256];
    
    while(1){
        memset(buffer, 0, sizeof(buffer));
        
        int client_sock = accept(server_fd, (struct sockaddr *)client_addr, addr_len);
        if(client_sock < 0){
            fprintf(stderr, "HeartBeat: Accept failed - %s\n", strerror(errno));
            continue;
        }

        SSL *ssl = NULL;
        if (hb_args->tls_enabled && hb_args->tls_ctx) {
            ssl = tls_accept_connection(hb_args->tls_ctx, client_sock);
            if (!ssl) {
                close(client_sock);
                continue;
            }
        }

        int bytes_recv = (int)tls_read_data(ssl, client_sock, buffer, sizeof(buffer) - 1);
        if (bytes_recv < 0) {
            fprintf(stderr, "HeartBeat: Recv failed - %s\n", strerror(errno));
            if (ssl) {
                tls_close_connection(ssl);
            }
            close(client_sock);
            continue;
        }

        buffer[bytes_recv] = '\0';
        LOG_INFO("Heartbeat received from %s: %s", inet_ntoa(client_addr->sin_addr), buffer);
        update_last_seen(buffer, last_seen_path);
        if (ssl) {
            tls_close_connection(ssl);
        }
        close(client_sock);
    }

    free(hb_args->client_addr);
    free(hb_args->addr_len);
    free(hb_args);
    pthread_exit(NULL);
    return NULL;
}

/*
 * Update the last seen timestamp for a data server in the CSV file
 * Format: "HEARTBEAT DS_ID <port>"
 */
void update_last_seen(const char *buffer, const char *last_seen_file){
    if (!buffer || !last_seen_file) {
        return;
    }

    int port = -1;

    /* Parse: "HEARTBEAT DS_ID <port>" */
    if (sscanf(buffer, "HEARTBEAT DS_ID %d", &port) != 1) {
        fprintf(stderr, "HeartBeat: Failed to parse heartbeat message: %s\n", buffer);
        return;
    }

    time_t now = time(NULL);
    
    /* Read all entries and update or add the port */
    FILE *fp_read = fopen(last_seen_file, "r");
    char tmp_path[DFS_PATH_LEN];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", last_seen_file);
    FILE *fp_write = fopen(tmp_path, "w");

    if (fp_write == NULL) {
        fprintf(stderr, "HeartBeat: Cannot open lastseen.csv for writing\n");
        if (fp_read) fclose(fp_read);
        return;
    }

    int found = 0;
    char line[256];

    if (fp_read != NULL) {
        while (fgets(line, sizeof(line), fp_read)) {
            if (line[0] == '#') {
                /* Copy comment lines */
                fputs(line, fp_write);
                continue;
            }

            int existing_port;
            long existing_time;
            
            if (sscanf(line, "%d,%ld", &existing_port, &existing_time) == 2) {
                if (existing_port == port) {
                    /* Update existing entry */
                    fprintf(fp_write, "%d,%ld\n", port, (long)now);
                    found = 1;
                } else {
                    /* Keep other entries */
                    fputs(line, fp_write);
                }
            }
        }
        fclose(fp_read);
    }

    /* If port not found, add new entry */
    if (!found) {
        fprintf(fp_write, "%d,%ld\n", port, (long)now);
    }

    fclose(fp_write);

    /* Replace original file with temp file */
    if (rename(tmp_path, last_seen_file) != 0) {
        fprintf(stderr, "HeartBeat: Failed to update %s\n", last_seen_file);
        perror("rename");
    }

    printf("[HeartBeat] Updated last seen for DS port %d: %ld\n", port, (long)now);
}
