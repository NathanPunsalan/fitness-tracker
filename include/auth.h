#ifndef AUTH_H
#define AUTH_H

#include <string>
#include <crow.h>
#include <database.h>

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

// Extracts the session token from the request Cookie header
bool getSessionTokenFromCookie(
    const std::string& cookieHeader,
    std::string& sessionToken
);

// Authenticates a request using the user's session cookie
bool authenticateRequest(
    const crow::request& request,
    Database& database,
    int& userId,
    std::string& errorMessage
);

#endif