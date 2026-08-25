#include "auth.h"

#include <sodium.h>

#include <stdexcept>
#include <string>

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