#include "common/auth.h"
#include "common/logging.h"
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <string.h>
#include <stdio.h>

int hash_password(const char *password, const char *salt, char *hash_out, size_t hash_len) {
    if (!password || !salt || !hash_out || hash_len < HASH_LENGTH) {
        return -1;
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    char combined[512];
    
    snprintf(combined, sizeof(combined), "%s%s", salt, password);
    
    SHA256((unsigned char *)combined, strlen(combined), hash);
    
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        snprintf(hash_out + (i * 2), 3, "%02x", hash[i]);
    }
    hash_out[SHA256_DIGEST_LENGTH * 2] = '\0';
    
    return 0;
}

int generate_salt(char *salt_out, size_t salt_len) {
    if (!salt_out || salt_len < SALT_LENGTH) {
        return -1;
    }

    unsigned char random_bytes[16];
    if (RAND_bytes(random_bytes, sizeof(random_bytes)) != 1) {
        LOG_ERROR("Failed to generate random salt");
        return -1;
    }

    for (size_t i = 0; i < sizeof(random_bytes); i++) {
        snprintf(salt_out + (i * 2), 3, "%02x", random_bytes[i]);
    }
    salt_out[SALT_LENGTH - 1] = '\0';
    
    return 0;
}

int verify_password(const char *password, const char *salt, const char *stored_hash) {
    if (!password || !salt || !stored_hash) {
        return -1;
    }

    char computed_hash[HASH_LENGTH];
    if (hash_password(password, salt, computed_hash, sizeof(computed_hash)) != 0) {
        return -1;
    }

    return (strcmp(computed_hash, stored_hash) == 0) ? 1 : 0;
}

int generate_secure_token(char *token_out, size_t token_len) {
    if (!token_out || token_len < 33) {
        return -1;
    }

    unsigned char random_bytes[16];
    if (RAND_bytes(random_bytes, sizeof(random_bytes)) != 1) {
        LOG_ERROR("Failed to generate secure token");
        return -1;
    }

    for (size_t i = 0; i < sizeof(random_bytes); i++) {
        snprintf(token_out + (i * 2), 3, "%02x", random_bytes[i]);
    }
    token_out[32] = '\0';
    
    return 0;
}
