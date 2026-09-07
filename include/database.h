#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>

// Handles the SQLite database connection for the app
class Database {
private:
    // Pointer to the active SQLite database connection
    sqlite3* db;

    // Location of the SQLite database file
    std::string databasePath;

    // Safely binds a string value to a placeholder in a prepared SQL statement
    bool bindText(
        sqlite3_stmt* statement,
        int index,
        const std::string& value
    );

public:
    // Creates a database object using the provided database file path
    Database(const std::string& path);

    // Ensures the database connection is closed when the object is destroyed
    ~Database();

    // Initializes the database connection and required schema
    bool initialize();

    // Opens the SQLite database connection
    bool connect();

    // Creates the initial database tables if they do not already exist
    bool initializeSchema();

    // Closes the SQLite database connection
    void disconnect();

    // Creates new user account in database
    // Returns true if user account was created successfully
    bool createUser(
        const std::string& username,
        const std::string& email,
        const std::string& passwordHash
    );

    // Retrieves a user's password hash using their username or email
    bool getUserLoginData(
        const std::string& login,
        int& userId,
        std::string& passwordHash
    );

    // Creates new authenticated session for user
    bool createSession(
        int userId,
        const std::string& sessionToken,
        const std::string& expiresAt
    );
};

#endif
