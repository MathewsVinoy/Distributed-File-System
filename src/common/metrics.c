#include "common/metrics.h"
#include "common/logging.h"
#include <string.h>
#include <stdio.h>

int metrics_init(metrics_t *metrics) {
    if (!metrics) {
        return -1;
    }

    memset(metrics, 0, sizeof(metrics_t));
    metrics->start_time = time(NULL);
    
    if (pthread_mutex_init(&metrics->lock, NULL) != 0) {
        LOG_ERROR("Failed to initialize metrics mutex");
        return -1;
    }

    return 0;
}

void metrics_record_request(metrics_t *metrics, int success, size_t bytes_transferred, int is_read) {
    if (!metrics) {
        return;
    }

    pthread_mutex_lock(&metrics->lock);
    
    metrics->total_requests++;
    if (success) {
        metrics->successful_requests++;
    } else {
        metrics->failed_requests++;
    }

    if (is_read) {
        metrics->bytes_read += bytes_transferred;
    } else {
        metrics->bytes_written += bytes_transferred;
    }

    metrics->last_request_time = time(NULL);
    
    pthread_mutex_unlock(&metrics->lock);
}

void metrics_get_snapshot(metrics_t *metrics, metrics_t *snapshot) {
    if (!metrics || !snapshot) {
        return;
    }

    pthread_mutex_lock(&metrics->lock);
    memcpy(snapshot, metrics, sizeof(metrics_t));
    pthread_mutex_unlock(&metrics->lock);
}

time_t metrics_get_uptime(metrics_t *metrics) {
    if (!metrics) {
        return 0;
    }

    pthread_mutex_lock(&metrics->lock);
    time_t uptime = time(NULL) - metrics->start_time;
    pthread_mutex_unlock(&metrics->lock);

    return uptime;
}

int metrics_to_json(metrics_t *metrics, char *json_out, size_t json_len) {
    if (!metrics || !json_out || json_len == 0) {
        return -1;
    }

    metrics_t snapshot;
    metrics_get_snapshot(metrics, &snapshot);

    time_t uptime = time(NULL) - snapshot.start_time;
    double success_rate = (snapshot.total_requests > 0) 
        ? (double)snapshot.successful_requests / snapshot.total_requests * 100.0 
        : 0.0;

    int written = snprintf(json_out, json_len,
        "{"
        "\"status\":\"healthy\","
        "\"uptime_seconds\":%ld,"
        "\"total_requests\":%lu,"
        "\"successful_requests\":%lu,"
        "\"failed_requests\":%lu,"
        "\"success_rate\":%.2f,"
        "\"bytes_read\":%lu,"
        "\"bytes_written\":%lu,"
        "\"last_request_time\":%ld"
        "}",
        (long)uptime,
        (unsigned long)snapshot.total_requests,
        (unsigned long)snapshot.successful_requests,
        (unsigned long)snapshot.failed_requests,
        success_rate,
        (unsigned long)snapshot.bytes_read,
        (unsigned long)snapshot.bytes_written,
        (long)snapshot.last_request_time
    );

    return (written > 0 && (size_t)written < json_len) ? 0 : -1;
}

void metrics_cleanup(metrics_t *metrics) {
    if (!metrics) {
        return;
    }

    pthread_mutex_destroy(&metrics->lock);
}
