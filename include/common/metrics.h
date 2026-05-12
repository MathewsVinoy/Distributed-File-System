#ifndef DFS_METRICS_H
#define DFS_METRICS_H

#include <stdint.h>
#include <time.h>
#include <pthread.h>

typedef struct {
    uint64_t total_requests;
    uint64_t successful_requests;
    uint64_t failed_requests;
    uint64_t bytes_read;
    uint64_t bytes_written;
    time_t start_time;
    time_t last_request_time;
    pthread_mutex_t lock;
} metrics_t;

/**
 * Initialize metrics system
 */
int metrics_init(metrics_t *metrics);

/**
 * Record a request
 */
void metrics_record_request(metrics_t *metrics, int success, size_t bytes_transferred, int is_read);

/**
 * Get current metrics snapshot
 */
void metrics_get_snapshot(metrics_t *metrics, metrics_t *snapshot);

/**
 * Generate metrics JSON string
 */
int metrics_to_json(metrics_t *metrics, char *json_out, size_t json_len);

/**
 * Calculate uptime in seconds
 */
time_t metrics_get_uptime(metrics_t *metrics);

/**
 * Cleanup metrics
 */
void metrics_cleanup(metrics_t *metrics);

#endif /* DFS_METRICS_H */
