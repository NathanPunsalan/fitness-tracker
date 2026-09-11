#ifndef VALIDATION_H
#define VALIDATION_H

#include <string>

// Returns true when a string is empty or contains only whitespace
bool isBlank(const std::string& value);

// Returns true when a username meets the application's basic requirements
bool isValidUsername(const std::string& username);

// Returns true when an email meets the application's basic format requirements
bool isValidEmail(const std::string& email);

// Returns true when a password meets the application's basic requirements
bool isValidPassword(const std::string& password);

#endif