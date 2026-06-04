#include "common/logging.h"
#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static FILE *log_stream = NULL;
static log_level_t current_level = LOG_LEVEL_INFO;

static const char *level_to_string(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO:  return "INFO";
        case LOG_LEVEL_WARN:  return "WARN";
        case LOG_LEVEL_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void log_init(FILE *stream) {
    pthread_mutex_lock(&log_mutex);
    log_stream = stream ? stream : stderr;
    pthread_mutex_unlock(&log_mutex);
}

void log_set_level(log_level_t level) {
    pthread_mutex_lock(&log_mutex);
    current_level = level;
    pthread_mutex_unlock(&log_mutex);
}

void log_message(log_level_t level, const char *file, int line, const char *fmt, ...) {
    if (level < current_level) {
        return;
    }

    pthread_mutex_lock(&log_mutex);
    if (!log_stream) {
        log_stream = stderr;
    }

    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tm_now);

    fprintf(log_stream, "%s [%s] (%s:%d) ", ts, level_to_string(level), file, line);

    va_list args;
    va_start(args, fmt);
    vfprintf(log_stream, fmt, args);
    va_end(args);

    fputc('\n', log_stream);
    fflush(log_stream);
    pthread_mutex_unlock(&log_mutex);
}

void log_shutdown(void) {
    pthread_mutex_lock(&log_mutex);
    if (log_stream && log_stream != stderr && log_stream != stdout) {
        fclose(log_stream);
    }
    log_stream = NULL;
    pthread_mutex_unlock(&log_mutex);
}
