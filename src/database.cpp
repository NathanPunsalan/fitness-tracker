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

    // Enable foreign key enforcement for the database connection
    int foreignKeyResult = sqlite3_exec(
        db,
        "PRAGMA foreign_keys = ON;",
        nullptr,
        nullptr,
        nullptr
    );

    // Check whether foreign key enforcement was enabled successfully
    if (foreignKeyResult != SQLITE_OK) {
        cerr << "Failed to enable foreign key enforcement: "
             << sqlite3_errmsg(db)
             << endl;

        disconnect();

        return false;
    }

    cout << "Database connected successfully." << endl;

    return true;
}

// Creates the required database tables if they do not already exist
bool Database::initializeSchema() {

    // All CREATE TABLE statements are executed together when the app starts.
    // IF NOT EXISTS prevents SQLite from replacing tables that already exist.
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
        ");"

        // Stores the general information for a completed combat-sports session.
        // Detailed combinations, drills, and rounds will use related tables later.
        "CREATE TABLE IF NOT EXISTS combat_sports_sessions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "discipline TEXT NOT NULL,"
        "training_type TEXT NOT NULL,"
        "session_date TEXT NOT NULL,"
        "duration_minutes INTEGER NOT NULL CHECK (duration_minutes > 0),"
        "recording_method TEXT NOT NULL DEFAULT 'manual' "
        "CHECK (recording_method IN ('manual', 'training_mode')),"
        "notes TEXT,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
        ");";

    // SQLite stores any schema error message in this pointer
    char* errorMessage = nullptr;

    // Execute all table-creation statements
    int result = sqlite3_exec(
        db,
        sql,
        nullptr,
        nullptr,
        &errorMessage
    );

    // Stop initialization if any table could not be created
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

// Creates a new user and stores their username, email, and hashed password
DatabaseResult Database::createUser(
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

        return DatabaseResult::Error;
    }

    // Bind the user account values to the SQL placeholders
    if (!bindText(statement, 1, username) ||
        !bindText(statement, 2, email) ||
        !bindText(statement, 3, passwordHash)) {

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    // Execute the prepared INSERT statement
    result = sqlite3_step(statement);

    sqlite3_finalize(statement);

    // User account was created successfully
    if (result == SQLITE_DONE) {
        return DatabaseResult::Success;
    }

    // A database constraint was violated
    if (result == SQLITE_CONSTRAINT) {
        cerr << "User creation conflict: "
             << sqlite3_errmsg(db) << endl;

        return DatabaseResult::Conflict;
    }

    // Any other SQLite result represents an unexpected database error
    cerr << "Failed to create user: "
         << sqlite3_errmsg(db) << endl;

    return DatabaseResult::Error;
}

// Retrieves a user's password hash using username or email
DatabaseResult Database::getUserLoginData(
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

        return DatabaseResult::Error;
    }

    // Bind the login value to both the username and email placeholders
    if (!bindText(statement, 1, login) ||
        !bindText(statement, 2, login)) {

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
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

        // Ensure SQLite returned a valid password hash
        if (storedHash == nullptr) {
            cerr << "User login data contained a null password hash." << endl;

            sqlite3_finalize(statement);
            return DatabaseResult::Error;
        }

        passwordHash =
            reinterpret_cast<const char*>(storedHash);

        sqlite3_finalize(statement);

        return DatabaseResult::Success;
    }

    // SQLITE_DONE means the query completed but no matching user was found
    if (result == SQLITE_DONE) {
        sqlite3_finalize(statement);

        return DatabaseResult::NotFound;
    }

    // Any other result represents an unexpected database error
    cerr << "Failed to retrieve user login data: "
         << sqlite3_errmsg(db) << endl;

    sqlite3_finalize(statement);

    return DatabaseResult::Error;
}

// Creates a new authenticated session for a user
DatabaseResult Database::createSession(
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

        return DatabaseResult::Error;
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
        return DatabaseResult::Error;
    }

    // Bind the session token and expiration time
    if (!bindText(statement, 2, sessionToken) ||
        !bindText(statement, 3, expiresAt)) {

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    // Execute the INSERT statement
    result = sqlite3_step(statement);

    sqlite3_finalize(statement);

    if (result != SQLITE_DONE) {
        cerr << "Failed to create session: "
             << sqlite3_errmsg(db) << endl;

        return DatabaseResult::Error;
    }

    return DatabaseResult::Success;
}

// Deletes an authenticated session using its session token
DatabaseResult Database::deleteSession(const string& sessionToken) {
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

        return DatabaseResult::Error;
    }

    // Bind the session token to the SQL placeholder
    if (!bindText(statement, 1, sessionToken)) {
        sqlite3_finalize(statement);

        return DatabaseResult::Error;
    }

    // Execute the DELETE statement
    result = sqlite3_step(statement);

    if (result != SQLITE_DONE) {
        cerr << "Failed to delete session: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);

        return DatabaseResult::Error;
    }

    // Check whether a matching session was deleted
    int deletedRows = sqlite3_changes(db);

    sqlite3_finalize(statement);

    if (deletedRows == 0) {
        return DatabaseResult::NotFound;
    }

    return DatabaseResult::Success;
}

// Checks whether a session token belongs to a valid, unexpired session
DatabaseResult Database::validateSession(
    const string& sessionToken,
    int& userId
) {
    const char* sql =
        "SELECT user_id "
        "FROM sessions "
        "WHERE session_token = ? "
        "AND expires_at > CURRENT_TIMESTAMP;";

    sqlite3_stmt* statement = nullptr;

    // Prepare the session lookup statement
    int result = sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare session validation statement: "
             << sqlite3_errmsg(db) << endl;

        return DatabaseResult::Error;
    }

    // Bind the session token to the SQL placeholder
    if (!bindText(statement, 1, sessionToken)) {
        sqlite3_finalize(statement);

        return DatabaseResult::Error;
    }

    // Execute the query
    result = sqlite3_step(statement);

    // SQLITE_ROW means a valid session was found
    if (result == SQLITE_ROW) {
        userId = sqlite3_column_int(statement, 0);

        sqlite3_finalize(statement);

        return DatabaseResult::Success;
    }

    // SQLITE_DONE means no valid session was found
    if (result == SQLITE_DONE) {
        sqlite3_finalize(statement);

        return DatabaseResult::NotFound;
    }

    // Any other result represents an unexpected database error
    cerr << "Failed to validate session: "
         << sqlite3_errmsg(db) << endl;

    sqlite3_finalize(statement);

    return DatabaseResult::Error;
}

// Creates a completed combat-sports training session for a user
DatabaseResult Database::createCombatSportsSession(
    int userId,
    const string& discipline,
    const string& trainingType,
    const string& sessionDate,
    int durationMinutes,
    const string& recordingMethod,
    const string& notes,
    int& sessionId
) {
    const char* sql =
        "INSERT INTO combat_sports_sessions ("
        "user_id, discipline, training_type, session_date, "
        "duration_minutes, recording_method, notes"
        ") VALUES (?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* statement = nullptr;

    // Convert the SQL text into a prepared statement
    int result = sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &statement,
        nullptr
    );

    // Stop if SQLite could not prepare the INSERT statement
    if (result != SQLITE_OK) {
        cerr << "Failed to prepare combat sports session insert statement: "
             << sqlite3_errmsg(db) << endl;

        return DatabaseResult::Error;
    }

    // Bind the user ID to the first SQL placeholder
    result = sqlite3_bind_int(
        statement,
        1,
        userId
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to bind combat sports session user ID: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    // Bind the text values that appear before the duration
    if (!bindText(statement, 2, discipline) ||
        !bindText(statement, 3, trainingType) ||
        !bindText(statement, 4, sessionDate)) {

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    // Bind the session duration to the fifth SQL placeholder
    result = sqlite3_bind_int(
        statement,
        5,
        durationMinutes
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to bind combat sports session duration: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    // Bind the remaining session information
    if (!bindText(statement, 6, recordingMethod) ||
        !bindText(statement, 7, notes)) {

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    // Execute the prepared INSERT statement
    result = sqlite3_step(statement);

    sqlite3_finalize(statement);

    // Store the automatically generated ID after a successful insert
    if (result == SQLITE_DONE) {
        sessionId = static_cast<int>(
            sqlite3_last_insert_rowid(db)
        );

        return DatabaseResult::Success;
    }

    // A foreign-key or CHECK constraint was violated
    if (result == SQLITE_CONSTRAINT) {
        cerr << "Combat sports session creation conflict: "
             << sqlite3_errmsg(db) << endl;

        return DatabaseResult::Conflict;
    }

    // Any other result represents an unexpected database error
    cerr << "Failed to create combat sports session: "
         << sqlite3_errmsg(db) << endl;

    return DatabaseResult::Error;
}

// Retrieves all combat-sports sessions belonging to a user
DatabaseResult Database::getCombatSportsSessions(
    int userId,
    vector<CombatSportsSession>& sessions
) {
    const char* sql =
        "SELECT "
        "id, user_id, discipline, training_type, session_date, "
        "duration_minutes, recording_method, notes, created_at, updated_at "
        "FROM combat_sports_sessions "
        "WHERE user_id = ? "
        "ORDER BY session_date DESC, id DESC;";

    sqlite3_stmt* statement = nullptr;

    // Remove any old values before loading the latest database results
    sessions.clear();

    // Convert the SQL query into a prepared statement
    int result = sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare combat sports session lookup statement: "
             << sqlite3_errmsg(db) << endl;

        return DatabaseResult::Error;
    }

    // Only retrieve sessions belonging to the requested user
    result = sqlite3_bind_int(
        statement,
        1,
        userId
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to bind combat sports session user ID: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    // Continue reading rows until SQLite reports that the query is finished
    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        CombatSportsSession session;

        // Retrieve the integer values from the current database row
        session.id = sqlite3_column_int(statement, 0);
        session.userId = sqlite3_column_int(statement, 1);
        session.durationMinutes = sqlite3_column_int(statement, 5);

        // Retrieve the required text values from the current database row
        session.discipline = reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 2)
        );

        session.trainingType = reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 3)
        );

        session.sessionDate = reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 4)
        );

        session.recordingMethod = reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 6)
        );

        // Notes may be NULL because the database column is optional
        const unsigned char* storedNotes =
            sqlite3_column_text(statement, 7);

        if (storedNotes != nullptr) {
            session.notes =
                reinterpret_cast<const char*>(storedNotes);
        }
        else {
            session.notes = "";
        }

        session.createdAt = reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 8)
        );

        session.updatedAt = reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 9)
        );

        // Add the completed session object to the output vector
        sessions.push_back(session);
    }

    // SQLITE_DONE means every matching row was read successfully
    if (result == SQLITE_DONE) {
        sqlite3_finalize(statement);

        return DatabaseResult::Success;
    }

    // Any other result represents an unexpected database error
    cerr << "Failed to retrieve combat sports sessions: "
         << sqlite3_errmsg(db) << endl;

    sqlite3_finalize(statement);

    // Prevent partially loaded results from being used after an error
    sessions.clear();

    return DatabaseResult::Error;
}

// Updates a combat-sports session belonging to a specific user
DatabaseResult Database::updateCombatSportsSession(
    int sessionId,
    int userId,
    const string& discipline,
    const string& trainingType,
    const string& sessionDate,
    int durationMinutes,
    const string& recordingMethod,
    const string& notes
) {
    const char* sql =
        "UPDATE combat_sports_sessions "
        "SET discipline = ?, "
        "training_type = ?, "
        "session_date = ?, "
        "duration_minutes = ?, "
        "recording_method = ?, "
        "notes = ?, "
        "updated_at = CURRENT_TIMESTAMP "
        "WHERE id = ? AND user_id = ?;";

    sqlite3_stmt* statement = nullptr;

    // Convert the SQL text into a prepared statement
    int result = sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare combat sports session update statement: "
             << sqlite3_errmsg(db) << endl;

        return DatabaseResult::Error;
    }

    // Bind the text values that will replace the existing session values
    if (!bindText(statement, 1, discipline) ||
        !bindText(statement, 2, trainingType) ||
        !bindText(statement, 3, sessionDate)) {

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    // Bind the updated session duration
    result = sqlite3_bind_int(
        statement,
        4,
        durationMinutes
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to bind updated combat sports session duration: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    // Bind the remaining updated session values
    if (!bindText(statement, 5, recordingMethod) ||
        !bindText(statement, 6, notes)) {

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    // Bind the session and user IDs used by the ownership check
    result = sqlite3_bind_int(
        statement,
        7,
        sessionId
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to bind combat sports session ID: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    result = sqlite3_bind_int(
        statement,
        8,
        userId
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to bind combat sports session owner ID: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    // Execute the prepared UPDATE statement
    result = sqlite3_step(statement);

    if (result == SQLITE_DONE) {
        // Check whether a session belonging to this user was updated
        int updatedRows = sqlite3_changes(db);

        sqlite3_finalize(statement);

        if (updatedRows == 0) {
            return DatabaseResult::NotFound;
        }

        return DatabaseResult::Success;
    }

    // A CHECK or foreign-key constraint was violated
    if (result == SQLITE_CONSTRAINT) {
        cerr << "Combat sports session update conflict: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Conflict;
    }

    // Any other result represents an unexpected database error
    cerr << "Failed to update combat sports session: "
         << sqlite3_errmsg(db) << endl;

    sqlite3_finalize(statement);

    return DatabaseResult::Error;
}

// Deletes a combat-sports session belonging to a specific user
DatabaseResult Database::deleteCombatSportsSession(
    int sessionId,
    int userId
) {
    const char* sql =
        "DELETE FROM combat_sports_sessions "
        "WHERE id = ? AND user_id = ?;";

    sqlite3_stmt* statement = nullptr;

    // Convert the SQL text into a prepared statement
    int result = sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare combat sports session delete statement: "
             << sqlite3_errmsg(db) << endl;

        return DatabaseResult::Error;
    }

    // Bind the session ID identifying the record to delete
    result = sqlite3_bind_int(
        statement,
        1,
        sessionId
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to bind combat sports session ID: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    // Bind the user ID to ensure the session belongs to that user
    result = sqlite3_bind_int(
        statement,
        2,
        userId
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to bind combat sports session owner ID: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    // Execute the prepared DELETE statement
    result = sqlite3_step(statement);

    if (result != SQLITE_DONE) {
        cerr << "Failed to delete combat sports session: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    // Check whether a session belonging to this user was deleted
    int deletedRows = sqlite3_changes(db);

    sqlite3_finalize(statement);

    if (deletedRows == 0) {
        return DatabaseResult::NotFound;
    }

    return DatabaseResult::Success;
}