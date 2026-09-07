#ifndef AUTH_H
#define AUTH_H

#include <string>

// Hashes a plain-text password using libsodium
std::string hashPassword(const std::string& password);

// Verifies password against a previously generated hash
bool verifyPassword(
    const std::string& password,
    const std::string& passwordHash
);

// Generates a cryptographically secure random token for user sessions
std::string generateSessionToken();

// Creates an expiration timestamp for a new session
std::string createSessionExpiration();

#endif