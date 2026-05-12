#ifndef DFS_CHECKSUM_H
#define DFS_CHECKSUM_H

#include <stddef.h>
#include <stdint.h>

#define CHECKSUM_LENGTH 65  // SHA256 hex string + null terminator

/**
 * Calculate SHA256 checksum of data
 * Returns 0 on success, -1 on failure
 */
int calculate_checksum(const void *data, size_t len, char *checksum_out, size_t checksum_len);

/**
 * Verify data against a checksum
 * Returns 1 if valid, 0 if invalid, -1 on error
 */
int verify_checksum(const void *data, size_t len, const char *expected_checksum);

/**
 * Calculate CRC32 checksum (faster for small blocks)
 * Returns the CRC32 value
 */
uint32_t calculate_crc32(const void *data, size_t len);

/**
 * Verify data against CRC32
 * Returns 1 if valid, 0 if invalid
 */
int verify_crc32(const void *data, size_t len, uint32_t expected_crc);

#endif /* DFS_CHECKSUM_H */
