#include "test/test.h"
#include "common/auth.h"
#include <string.h>

TEST(generate_salt) {
    char salt[SALT_LENGTH];
    int result = generate_salt(salt, sizeof(salt));
    
    ASSERT_EQUAL(0, result);
    ASSERT_TRUE(strlen(salt) > 0);
    
    // Two salts should be different
    char salt2[SALT_LENGTH];
    generate_salt(salt2, sizeof(salt2));
    ASSERT_FALSE(strcmp(salt, salt2) == 0);
}

TEST(hash_password) {
    const char *password = "test_password";
    char salt[SALT_LENGTH];
    char hash[HASH_LENGTH];
    
    generate_salt(salt, sizeof(salt));
    int result = hash_password(password, salt, hash, sizeof(hash));
    
    ASSERT_EQUAL(0, result);
    ASSERT_TRUE(strlen(hash) == 64);
}

TEST(verify_password_success) {
    const char *password = "my_secure_password";
    char salt[SALT_LENGTH];
    char hash[HASH_LENGTH];
    
    generate_salt(salt, sizeof(salt));
    hash_password(password, salt, hash, sizeof(hash));
    
    int result = verify_password(password, salt, hash);
    ASSERT_EQUAL(1, result);
}

TEST(verify_password_failure) {
    const char *password = "correct_password";
    const char *wrong_password = "wrong_password";
    char salt[SALT_LENGTH];
    char hash[HASH_LENGTH];
    
    generate_salt(salt, sizeof(salt));
    hash_password(password, salt, hash, sizeof(hash));
    
    int result = verify_password(wrong_password, salt, hash);
    ASSERT_EQUAL(0, result);
}

TEST(same_password_different_salt) {
    const char *password = "password123";
    char salt1[SALT_LENGTH];
    char salt2[SALT_LENGTH];
    char hash1[HASH_LENGTH];
    char hash2[HASH_LENGTH];
    
    generate_salt(salt1, sizeof(salt1));
    generate_salt(salt2, sizeof(salt2));
    
    hash_password(password, salt1, hash1, sizeof(hash1));
    hash_password(password, salt2, hash2, sizeof(hash2));
    
    // Same password with different salts should produce different hashes
    ASSERT_FALSE(strcmp(hash1, hash2) == 0);
}

TEST(generate_secure_token) {
    char token1[33];
    char token2[33];
    
    int result = generate_secure_token(token1, sizeof(token1));
    ASSERT_EQUAL(0, result);
    ASSERT_TRUE(strlen(token1) == 32);
    
    generate_secure_token(token2, sizeof(token2));
    ASSERT_FALSE(strcmp(token1, token2) == 0);
}

TEST_MAIN() {
    printf("\n=== Authentication Tests ===\n\n");
    
    RUN_TEST(generate_salt);
    RUN_TEST(hash_password);
    RUN_TEST(verify_password_success);
    RUN_TEST(verify_password_failure);
    RUN_TEST(same_password_different_salt);
    RUN_TEST(generate_secure_token);
    
    TEST_SUMMARY();
}
