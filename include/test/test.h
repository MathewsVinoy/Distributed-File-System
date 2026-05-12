#ifndef DFS_TEST_H
#define DFS_TEST_H

#include <stdio.h>
#include <string.h>

/* Test framework macros */
#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("Running %s... ", #name); \
    fflush(stdout); \
    test_##name(); \
    printf("PASSED\n"); \
    tests_passed++; \
} while(0)

#define ASSERT_TRUE(condition) do { \
    if (!(condition)) { \
        printf("FAILED\n"); \
        printf("  Assertion failed: %s\n", #condition); \
        printf("  File: %s, Line: %d\n", __FILE__, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_FALSE(condition) ASSERT_TRUE(!(condition))

#define ASSERT_EQUAL(expected, actual) do { \
    if ((expected) != (actual)) { \
        printf("FAILED\n"); \
        printf("  Expected: %d, Got: %d\n", (int)(expected), (int)(actual)); \
        printf("  File: %s, Line: %d\n", __FILE__, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_STR_EQUAL(expected, actual) do { \
    if (strcmp((expected), (actual)) != 0) { \
        printf("FAILED\n"); \
        printf("  Expected: \"%s\", Got: \"%s\"\n", (expected), (actual)); \
        printf("  File: %s, Line: %d\n", __FILE__, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_NOT_NULL(ptr) do { \
    if ((ptr) == NULL) { \
        printf("FAILED\n"); \
        printf("  Pointer is NULL: %s\n", #ptr); \
        printf("  File: %s, Line: %d\n", __FILE__, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_NULL(ptr) do { \
    if ((ptr) != NULL) { \
        printf("FAILED\n"); \
        printf("  Pointer is not NULL: %s\n", #ptr); \
        printf("  File: %s, Line: %d\n", __FILE__, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

/* Global test counters */
extern int tests_passed;
extern int tests_failed;

/* Test runner */
#define TEST_MAIN() \
int tests_passed = 0; \
int tests_failed = 0; \
int main(void)

#define TEST_SUMMARY() do { \
    printf("\n=================================\n"); \
    printf("Test Summary:\n"); \
    printf("  Passed: %d\n", tests_passed); \
    printf("  Failed: %d\n", tests_failed); \
    printf("  Total:  %d\n", tests_passed + tests_failed); \
    printf("=================================\n"); \
    return (tests_failed > 0) ? 1 : 0; \
} while(0)

#endif /* DFS_TEST_H */
