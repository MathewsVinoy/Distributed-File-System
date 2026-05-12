#ifndef DFS_AUTH_H
#define DFS_AUTH_H

#include <stddef.h>

#define HASH_LENGTH 65  // SHA256 hex string (64 chars) + null terminator
#define SALT_LENGTH 33  // 32 hex chars + null terminator

/**
 * Hash a password using SHA-256 with salt
 * Returns 0 on success, -1 on failure
 */
int hash_password(const char *password, const char *salt, char *hash_out, size_t hash_len);

/**
 * Generate a random salt
 * Returns 0 on success, -1 on failure
 */
int generate_salt(char *salt_out, size_t salt_len);

/**
 * Verify a password against a stored hash
 * Returns 1 if match, 0 if no match, -1 on error
 */
int verify_password(const char *password, const char *salt, const char *stored_hash);

/**
 * Generate a cryptographically secure random token
 * Returns 0 on success, -1 on failure
 */
int generate_secure_token(char *token_out, size_t token_len);

#endif /* DFS_AUTH_H */
