#include "auth.h"

#include <sodium.h>

#include <stdexcept>
#include <string>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

using namespace std;

// Hashes password using libsodium
string hashPassword(const string& password)
{
    // Create a buffer large enough to store the complete password hash
    char passwordHash[crypto_pwhash_STRBYTES];

    // Generate a secure password hash using libsodium
    if (crypto_pwhash_str(
        passwordHash,
        password.c_str(),
        password.length(),
        crypto_pwhash_OPSLIMIT_INTERACTIVE,
        crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0)
    {
        // Hash fail safe
        throw runtime_error("Unable to hash password.");
    }

    // Convert the character buffer into a C++ string
    return string(passwordHash);
}

// Verifies a password against a previously generated hash
bool verifyPassword(
    const string& password,
    const string& passwordHash)
{
    // Libsodium returns 0 when the password matches the stored hash
    return crypto_pwhash_str_verify(
        passwordHash.c_str(),
        password.c_str(),
        password.length()) == 0;
}

string generateSessionToken() {
    // 32 random bytes provides 256 bits of randomness
    unsigned char randomBytes[32];

    // Generate random bytes using libsodium's secure random number generator
    randombytes_buf(
        randomBytes,
        sizeof(randomBytes)
    );

    // Each byte becomes two hexadecimal characters,
    // Plus one extra character for the null terminator
    char tokenHex[65];

    // Convert the random bytes into a hexadecimal string
    sodium_bin2hex(
        tokenHex,
        sizeof(tokenHex),
        randomBytes,
        sizeof(randomBytes)
    );

    return string(tokenHex);
}

// Creates an expiration timestamp for a new session
string createSessionExpiration() {
    // Set the session to expire 24 hrs from now
    auto expirationTime =
        chrono::system_clock::now() + chrono::hours(24);

    time_t expiration =
        chrono::system_clock::to_time_t(expirationTime);

    tm utcTime{};

    // Convert the expiration time to UTC
    gmtime_s(&utcTime, &expiration);

    // Format the timestamp so SQLite can store and compare easily
    ostringstream output;

    output << put_time(
        &utcTime,
        "%Y-%m-%d %H:%M:%S"
    );

    return output.str();
}