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

#endif