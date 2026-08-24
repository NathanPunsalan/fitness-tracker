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
};

#endif
