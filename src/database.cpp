#include "database.h"

#include <iostream>

using namespace std;

// Stores the database path and starts with no active database connection
Database::Database(const string& path) {
    databasePath = path;
    db = nullptr;
}

// Automatically closes the database connection when the object is destroyed
Database::~Database() {
    disconnect();
}

// Opens the SQLite database
bool Database::connect() {

    // sqlite3_open creates the database file if it does not already exist
    int result = sqlite3_open(databasePath.c_str(), &db);

    // SQLITE_OK means the database opened successfully
    if (result != SQLITE_OK) {
        cerr << "Database connection error: "
                  << sqlite3_errmsg(db)
                  << endl;

        // Close the connection if SQLite partially opened the database
        disconnect();

        return false;
    }

    // Enable foreign key enforcement for database connection
    int foreignKeyResult = sqlite3_exec(
        db,
        "PRAGMA foreign_keys = ON;",
        nullptr,
        nullptr,
        nullptr
    );

    // Check whether foreign key enforcement was enabled successfully
    if (foreignKeyResult != SQLITE_OK)  {
        cerr << "Failed to enable foreign key enforcement: "
             << sqlite3_errmsg(db)
             << endl;

        disconnect();

        return false;
    }

    cout << "Database connected successfully." << endl;

    return true;
}

// Creates the initial database tables if they do not already exist
bool Database::initializeSchema() {

    // SQL statement used to create tables
    const char* sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT NOT NULL UNIQUE,"
        "email TEXT NOT NULL UNIQUE,"
        "password_hash TEXT NOT NULL,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
        ");";

    // SQLite stores any error message from sqlite3_exec here
    char* errorMessage = nullptr;

    // Execute the SQL statement
    int result = sqlite3_exec(db, sql, nullptr, nullptr, &errorMessage);

    // Check whether the schema was created successfully
    if (result != SQLITE_OK) {
        cerr << "Database schema error: "
             << errorMessage
             << endl;

        sqlite3_free(errorMessage);

        return false;
    }

    cout << "Database schema initialized successfully." << endl;

    return true;
}

// Closes the SQLite database if a connection is currently open
void Database::disconnect() {

    if (db != nullptr) {
        sqlite3_close(db);
        db = nullptr;

        cout << "Database connection closed." << endl;
    }
}