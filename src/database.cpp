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

// Function to bind a string value to a placeholder in a prepared SQL statement
bool Database::bindText(
    sqlite3_stmt* statement,
    int index,
    const string& value
) {
    int result = sqlite3_bind_text(
        statement,
        index,
        value.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    // SQLITE_OK means the value was bound successfully
    if (result != SQLITE_OK) {
        cerr << "Failed to bind SQL text value: "
             << sqlite3_errmsg(db) << endl;

        return false;
    }

    return true;
}

// Initializes the database connection and required schema
bool Database::initialize() {

    // Open the SQLite database and enable foreign key enforcement
    if (!connect()) {
        return false;
    }

    // Create any required database tables that do not already exist
    if (!initializeSchema()) {
        disconnect();
        return false;
    }

    return true;
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
        ");"

        "CREATE TABLE IF NOT EXISTS sessions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "session_token TEXT NOT NULL UNIQUE,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "expires_at TEXT NOT NULL,"
        "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
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

// Creates new user and stores their username, email, and hashed password
bool Database::createUser(
    const string& username,
    const string& email,
    const string& passwordHash
) {
    const char* sql =
        "INSERT INTO users (username, email, password_hash) "
        "VALUES (?, ?, ?);";

    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &statement,
        nullptr
    );

    // Stop if SQLite could not prepare the statement
    if (result != SQLITE_OK) {
        cerr << "Failed to prepare user insert statement: "
             << sqlite3_errmsg(db) << endl;

        return false;
    }

    // Bind the user account values to the SQL placeholders
    if (!bindText(statement, 1, username) ||
        !bindText(statement, 2, email) ||
        !bindText(statement, 3, passwordHash)) {

            sqlite3_finalize(statement);
            return false;
        }

    // Execute the prepared INSERT statement
    result = sqlite3_step(statement);

    sqlite3_finalize(statement);

    // SQLITE_DONE means the INSERT completed successfully
    if (result != SQLITE_DONE) {
        cerr << "Failed to create user: "
             << sqlite3_errmsg(db) << endl;

        return false;
    }

    return true;
}

// Retrieves a user's password hash using username or email
bool Database::getUserLoginData(
    const string& login,
    int& userId,
    string& passwordHash
) {
    // Search for a user whose username or email matches the submitted login value
    const char* sql =
        "SELECT id, password_hash "
        "FROM users "
        "WHERE username = ? OR email = ?;";

    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare user lookup statement: "
             << sqlite3_errmsg(db) << endl;

        return false;
    }

    // Bind the login value to both the username and email placeholders
    if (!bindText(statement, 1, login) ||
        !bindText(statement, 2, login)) {
           
            sqlite3_finalize(statement);
            return false;
        }

    // Execute query
    result = sqlite3_step(statement);

    // SQLITE_ROW means that a matching user was found
    if (result == SQLITE_ROW) {

        // Retrieve user ID from the first selected column
        userId = sqlite3_column_int(statement, 0);

        // Retrieve the password hash from the second selected column
        const unsigned char* storedHash =
            sqlite3_column_text(statement, 1);

        // Ensures SQLite returned a valid value
        if (storedHash != nullptr) {
            passwordHash = 
                reinterpret_cast<const char*>(storedHash);
        }

        sqlite3_finalize(statement);

        return true;
    }

    // No matching user was found
    sqlite3_finalize(statement);

    return false;
}

// Creates a new authenticated session for a user
bool Database::createSession(
    int userId,
    const string& sessionToken,
    const string& expiresAt
) {
    const char* sql =
        "INSERT INTO sessions (user_id, session_token, expires_at) "
        "VALUES (?, ?, ?);";
    
    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare session insert statement: "
             << sqlite3_errmsg(db) << endl;

        return false;
    }

    // Bind the user ID to the first placeholder
    result = sqlite3_bind_int(
        statement,
        1,
        userId
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to bind session user ID: "
             << sqlite3_errmsg(db) << endl;
            
        sqlite3_finalize(statement);
        return false;
    }

    // Bind the session token and expiration time
    if (!bindText(statement, 2, sessionToken) ||
        !bindText(statement, 3, expiresAt)) {

            sqlite3_finalize(statement);
            return false;
        }

    // Execute the INSERT statement
    result = sqlite3_step(statement);

    sqlite3_finalize(statement);

    if (result != SQLITE_DONE) {
        cerr << "Failed to create session: "
             << sqlite3_errmsg(db) << endl;

        return false;
    }

    return true;
}

// Deletes an authenticated session using its session token
bool Database::deleteSession(const string& sessionToken) {
    const char* sql =
        "DELETE FROM sessions "
        "WHERE session_token = ?;";

    sqlite3_stmt* statement = nullptr;

    // Prepare the DELETE statement
    int result = sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare session delete statement: "
             << sqlite3_errmsg(db) << endl;
        
        return false;
    }

    // Bind the session token to the SQL placeholder
    if (!bindText(statement, 1, sessionToken)) {
        sqlite3_finalize(statement);
        
        return false;
    }

    // Execute the DELETE statement
    result = sqlite3_step(statement);

    sqlite3_finalize(statement);

    // SQLITE_DONE means SQLite successfully executed the DELETE
    if (result != SQLITE_DONE) {
        cerr << "Failed to delete session: "
             << sqlite3_errmsg(db) << endl;

        return false;
    }

    return true;
}