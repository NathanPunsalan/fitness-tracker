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

    std::cout << "Database connected successfully." << endl;

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