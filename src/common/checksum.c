#include "common/checksum.h"
#include "common/logging.h"
#include <openssl/sha.h>
#include <string.h>
#include <stdio.h>

int calculate_checksum(const void *data, size_t len, char *checksum_out, size_t checksum_len) {
    if (!data || !checksum_out || checksum_len < CHECKSUM_LENGTH) {
        return -1;
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char *)data, len, hash);

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        snprintf(checksum_out + (i * 2), 3, "%02x", hash[i]);
    }
    checksum_out[SHA256_DIGEST_LENGTH * 2] = '\0';

    return 0;
}

int verify_checksum(const void *data, size_t len, const char *expected_checksum) {
    if (!data || !expected_checksum) {
        return -1;
    }

    char computed[CHECKSUM_LENGTH];
    if (calculate_checksum(data, len, computed, sizeof(computed)) != 0) {
        return -1;
    }

    return (strcmp(computed, expected_checksum) == 0) ? 1 : 0;
}

/* CRC32 lookup table */
static uint32_t crc32_table[256];
static int crc32_table_initialized = 0;

static void init_crc32_table(void) {
    uint32_t polynomial = 0xEDB88320;
    
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ polynomial;
            } else {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }
    crc32_table_initialized = 1;
}

uint32_t calculate_crc32(const void *data, size_t len) {
    if (!data || len == 0) {
        return 0;
    }

    if (!crc32_table_initialized) {
        init_crc32_table();
    }

    uint32_t crc = 0xFFFFFFFF;
    const unsigned char *bytes = (const unsigned char *)data;

    for (size_t i = 0; i < len; i++) {
        uint8_t index = (uint8_t)(crc ^ bytes[i]);
        crc = (crc >> 8) ^ crc32_table[index];
    }

    return crc ^ 0xFFFFFFFF;
}

int verify_crc32(const void *data, size_t len, uint32_t expected_crc) {
    if (!data) {
        return 0;
    }

    uint32_t computed = calculate_crc32(data, len);
    return (computed == expected_crc) ? 1 : 0;
}
