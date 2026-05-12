#ifndef DFS_RETRY_H
#define DFS_RETRY_H

#include <stddef.h>

#define MAX_RETRY_ATTEMPTS 5
#define INITIAL_BACKOFF_MS 100
#define MAX_BACKOFF_MS 10000

typedef struct {
    int max_attempts;
    int initial_backoff_ms;
    int max_backoff_ms;
    double backoff_multiplier;
    int current_attempt;
} retry_config_t;

/**
 * Initialize retry configuration with defaults
 */
void retry_config_init(retry_config_t *config);

/**
 * Execute a function with retry logic
 * func: Function to retry (should return 0 on success, non-zero on failure)
 * arg: Argument to pass to the function
 * config: Retry configuration
 * Returns: 0 on success, -1 if all retries exhausted
 */
int retry_execute(int (*func)(void *), void *arg, retry_config_t *config);

/**
 * Calculate backoff time for current attempt
 * Returns: backoff time in milliseconds
 */
int retry_calculate_backoff(const retry_config_t *config);

/**
 * Sleep for specified milliseconds
 */
void retry_sleep_ms(int milliseconds);

#endif /* DFS_RETRY_H */
