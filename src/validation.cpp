#include "validation.h"

#include <cctype>

using namespace std;

// Returns true when a string is empty or contains only whitespace
bool isBlank(const string& value) {
    // Check each character in the string
    for (char character : value) {
        // If non-whitespace is found, the string is not blank
        if (!isspace(static_cast<unsigned char>(character))) {
            return false;
        }
    }

    // An empty string or a string containing only whitespace is blank
    return true;
}

// Returns true when a username meets basic requirements
bool isValidUsername(const string& username) {
    // Reject usernames that are empty or contain only whitespace
    if (isBlank(username)) {
        return false;
    }

    // Reject usernames shorter than 5 characters
    if (username.length() < 5) {
        return false;
    }

    // Reject usernames longer than 15 characters
    if (username.length() > 15) {
        return false;
    }

    return true;
}

// Returns true when an email meets basic format requirements
bool isValidEmail(const string& email) {
    // Reject emails that are empty or contain only whitespace
    if (isBlank(email)) {
        return false;
    }

    // Find the position of the @ symbol
    size_t atPosition = email.find('@');

    // Reject emails without an @ symbol
    if (atPosition == string::npos) {
        return false;
    }

    // Reject emails that begin with @
    if (atPosition == 0) {
        return false;
    }

    // Reject emails containing more than one @ symbol
    if (email.find('@', atPosition + 1) != string::npos) {
        return false;
    }

    // Reject emails where the local part begins or ends with a period
    if (email[0] == '.' || email[atPosition - 1] == '.') {
        return false;
    }

    // Validate characters before the @ symbol
    for (size_t i = 0; i < atPosition; i++) {
        char character = email[i];

        // Allow letters, numbers, periods, underscores, hyphens, and plus signs
        if (!isalnum(static_cast<unsigned char>(character)) &&
            character != '.' &&
            character != '_' &&
            character != '-' &&
            character != '+') {
            return false;
        }
    }

    // Find a period after the @ symbol
    size_t dotPosition = email.find('.', atPosition + 1);

    // Reject emails without a period after @
    if (dotPosition == string::npos) {
        return false;
    }

    // Reject emails where the domain begins with a period
    if (email[atPosition + 1] == '.') {
        return false;
    }

    // Reject emails ending with a period
    if (email[email.length() - 1] == '.') {
        return false;
    }

    // Reject consecutive periods anywhere in the email
    if (email.find("..") != string::npos) {
        return false;
    }

    // Validate characters after the @ symbol
    for (size_t i = atPosition + 1; i < email.length(); i++) {
        char character = email[i];

        // Allow letters, numbers, periods, and hyphens
        if (!isalnum(static_cast<unsigned char>(character)) &&
            character != '.' &&
            character != '-') {
            return false;
        }
    }

    return true;
}

// Returns true when a password meets basic security requirements
bool isValidPassword(const string& password) {
    // Reject passwords that are empty or contain only whitespace
    if (isBlank(password)) {
        return false;
    }

    // Reject passwords shorter than 8 characters
    if (password.length() < 8) {
        return false;
    }

    bool hasUppercase = false;
    bool hasLowercase = false;
    bool hasNumber = false;
    bool hasSymbol = false;

    // Check each character in the password
    for (char character : password) {
        unsigned char currentCharacter =
            static_cast<unsigned char>(character);

        if (isupper(currentCharacter)) {
            hasUppercase = true;
        }
        else if (islower(currentCharacter)) {
            hasLowercase = true;
        }
        else if (isdigit(currentCharacter)) {
            hasNumber = true;
        }
        else if (!isspace(currentCharacter)) {
            hasSymbol = true;
        }
    }

    // Password is valid only if all required character types are present
    return hasUppercase &&
           hasLowercase &&
           hasNumber &&
           hasSymbol;
}