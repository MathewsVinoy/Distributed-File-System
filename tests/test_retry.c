#include "test/test.h"
#include "common/retry.h"
#include <time.h>

static int call_count = 0;
static int fail_times = 0;

int test_function_always_fail(void *arg) {
    (void)arg;
    call_count++;
    return -1;
}

int test_function_succeed_after_n(void *arg) {
    (void)arg;
    call_count++;
    if (call_count > fail_times) {
        return 0;
    }
    return -1;
}

int test_function_always_succeed(void *arg) {
    (void)arg;
    call_count++;
    return 0;
}

TEST(retry_immediate_success) {
    retry_config_t config;
    retry_config_init(&config);
    
    call_count = 0;
    int result = retry_execute(test_function_always_succeed, NULL, &config);
    
    ASSERT_EQUAL(0, result);
    ASSERT_EQUAL(1, call_count);
}

TEST(retry_fail_all_attempts) {
    retry_config_t config;
    retry_config_init(&config);
    config.max_attempts = 3;
    config.initial_backoff_ms = 10;  // Fast for testing
    
    call_count = 0;
    int result = retry_execute(test_function_always_fail, NULL, &config);
    
    ASSERT_EQUAL(-1, result);
    ASSERT_EQUAL(3, call_count);
}

TEST(retry_succeed_after_retries) {
    retry_config_t config;
    retry_config_init(&config);
    config.max_attempts = 5;
    config.initial_backoff_ms = 10;
    
    call_count = 0;
    fail_times = 2;  // Fail twice, then succeed
    int result = retry_execute(test_function_succeed_after_n, NULL, &config);
    
    ASSERT_EQUAL(0, result);
    ASSERT_EQUAL(3, call_count);
}

TEST(retry_backoff_calculation) {
    retry_config_t config;
    retry_config_init(&config);
    config.initial_backoff_ms = 100;
    config.backoff_multiplier = 2.0;
    config.max_backoff_ms = 5000;
    
    config.current_attempt = 0;
    int backoff0 = retry_calculate_backoff(&config);
    ASSERT_EQUAL(0, backoff0);
    
    config.current_attempt = 1;
    int backoff1 = retry_calculate_backoff(&config);
    ASSERT_TRUE(backoff1 >= 100 && backoff1 <= 125);
    
    config.current_attempt = 2;
    int backoff2 = retry_calculate_backoff(&config);
    ASSERT_TRUE(backoff2 >= 200 && backoff2 <= 250);
}

TEST(retry_max_backoff_limit) {
    retry_config_t config;
    retry_config_init(&config);
    config.initial_backoff_ms = 1000;
    config.backoff_multiplier = 2.0;
    config.max_backoff_ms = 2000;
    
    config.current_attempt = 10;
    int backoff = retry_calculate_backoff(&config);
    ASSERT_TRUE(backoff <= 2500);  // Max + 25% jitter
}

TEST_MAIN() {
    printf("\n=== Retry Logic Tests ===\n\n");
    
    RUN_TEST(retry_immediate_success);
    RUN_TEST(retry_fail_all_attempts);
    RUN_TEST(retry_succeed_after_retries);
    RUN_TEST(retry_backoff_calculation);
    RUN_TEST(retry_max_backoff_limit);
    
    TEST_SUMMARY();
}
