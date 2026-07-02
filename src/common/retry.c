#include "common/retry.h"
#include "common/logging.h"
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

void retry_config_init(retry_config_t *config) {
    if (!config) {
        return;
    }
    
    config->max_attempts = MAX_RETRY_ATTEMPTS;
    config->initial_backoff_ms = INITIAL_BACKOFF_MS;
    config->max_backoff_ms = MAX_BACKOFF_MS;
    config->backoff_multiplier = 2.0;
    config->current_attempt = 0;
}

int retry_calculate_backoff(const retry_config_t *config) {
    if (!config || config->current_attempt == 0) {
        return 0;
    }

    int backoff = config->initial_backoff_ms;
    
    for (int i = 1; i < config->current_attempt; i++) {
        backoff = (int)(backoff * config->backoff_multiplier);
        if (backoff > config->max_backoff_ms) {
            backoff = config->max_backoff_ms;
            break;
        }
    }

    /* Add jitter (0-25% random variation) */
    int jitter = (rand() % (backoff / 4));
    backoff += jitter;

    return (backoff > config->max_backoff_ms) ? config->max_backoff_ms : backoff;
}

void retry_sleep_ms(int milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

int retry_execute(int (*func)(void *), void *arg, retry_config_t *config) {
    if (!func || !config) {
        return -1;
    }

    srand((unsigned int)time(NULL));

    for (config->current_attempt = 0; config->current_attempt < config->max_attempts; config->current_attempt++) {
        int result = func(arg);
        
        if (result == 0) {
            if (config->current_attempt > 0) {
                LOG_INFO("Operation succeeded after %d retries", config->current_attempt);
            }
            return 0;
        }

        if (config->current_attempt < config->max_attempts - 1) {
            int backoff = retry_calculate_backoff(config);
            LOG_WARN("Operation failed (attempt %d/%d), retrying in %d ms",
                     config->current_attempt + 1, config->max_attempts, backoff);
            retry_sleep_ms(backoff);
        }
    }

    LOG_ERROR("Operation failed after %d attempts", config->max_attempts);
    return -1;
}
