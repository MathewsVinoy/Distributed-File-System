#include "test/test.h"
#include "common/checksum.h"
#include <string.h>

TEST(checksum_basic) {
    const char *data = "Hello, World!";
    char checksum1[CHECKSUM_LENGTH];
    char checksum2[CHECKSUM_LENGTH];
    
    int result = calculate_checksum(data, strlen(data), checksum1, sizeof(checksum1));
    ASSERT_EQUAL(0, result);
    ASSERT_TRUE(strlen(checksum1) == 64);
    
    // Same data should produce same checksum
    result = calculate_checksum(data, strlen(data), checksum2, sizeof(checksum2));
    ASSERT_EQUAL(0, result);
    ASSERT_STR_EQUAL(checksum1, checksum2);
}

TEST(checksum_verify) {
    const char *data = "Test data";
    char checksum[CHECKSUM_LENGTH];
    
    calculate_checksum(data, strlen(data), checksum, sizeof(checksum));
    
    int result = verify_checksum(data, strlen(data), checksum);
    ASSERT_EQUAL(1, result);
    
    // Different data should fail verification
    result = verify_checksum("Different data", 14, checksum);
    ASSERT_EQUAL(0, result);
}

TEST(checksum_different_data) {
    const char *data1 = "Data 1";
    const char *data2 = "Data 2";
    char checksum1[CHECKSUM_LENGTH];
    char checksum2[CHECKSUM_LENGTH];
    
    calculate_checksum(data1, strlen(data1), checksum1, sizeof(checksum1));
    calculate_checksum(data2, strlen(data2), checksum2, sizeof(checksum2));
    
    ASSERT_FALSE(strcmp(checksum1, checksum2) == 0);
}

TEST(crc32_basic) {
    const char *data = "Hello, CRC32!";
    uint32_t crc1 = calculate_crc32(data, strlen(data));
    uint32_t crc2 = calculate_crc32(data, strlen(data));
    
    ASSERT_EQUAL(crc1, crc2);
    ASSERT_TRUE(crc1 != 0);
}

TEST(crc32_verify) {
    const char *data = "Test CRC32";
    uint32_t crc = calculate_crc32(data, strlen(data));
    
    int result = verify_crc32(data, strlen(data), crc);
    ASSERT_EQUAL(1, result);
    
    result = verify_crc32("Wrong data", 10, crc);
    ASSERT_EQUAL(0, result);
}

TEST_MAIN() {
    printf("\n=== Checksum Tests ===\n\n");
    
    RUN_TEST(checksum_basic);
    RUN_TEST(checksum_verify);
    RUN_TEST(checksum_different_data);
    RUN_TEST(crc32_basic);
    RUN_TEST(crc32_verify);
    
    TEST_SUMMARY();
}
