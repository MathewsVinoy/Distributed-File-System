#ifndef DFS_LOGGING_H
#define DFS_LOGGING_H

#include <stdio.h>

typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} log_level_t;

void log_init(FILE *stream);
void log_set_level(log_level_t level);
void log_message(log_level_t level, const char *file, int line, const char *fmt, ...);
void log_shutdown(void);

#define LOG_DEBUG(...) log_message(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)  log_message(LOG_LEVEL_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)  log_message(LOG_LEVEL_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) log_message(LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)

#endif
