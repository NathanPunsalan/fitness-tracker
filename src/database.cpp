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

// Starts a write transaction for a multi-statement database operation
bool Database::beginTransaction() {
    char* errorMessage = nullptr;

    // Reserve write access before beginning the multi-statement operation.
    int result = sqlite3_exec(
        db,
        "BEGIN IMMEDIATE TRANSACTION;",
        nullptr,
        nullptr,
        &errorMessage
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to begin database transaction: "
             << (errorMessage != nullptr
                ? errorMessage
                : sqlite3_errmsg(db))
             << endl;

        if (errorMessage != nullptr) {
            sqlite3_free(errorMessage);
        }

        return false;
    }

    return true;
}

// Permanently saves every statement in the active transaction
bool Database::commitTransaction() {
    char* errorMessage = nullptr;

    int result = sqlite3_exec(
        db,
        "COMMIT;",
        nullptr,
        nullptr,
        &errorMessage
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to commit database transaction: "
             << (errorMessage != nullptr
                ? errorMessage
                : sqlite3_errmsg(db))
             << endl;

        if (errorMessage != nullptr) {
            sqlite3_free(errorMessage);
        }

        return false;
    }

    return true;
}

// Reverses every statement in the active transaction after a failure
void Database::rollbackTransaction() {
    char* errorMessage = nullptr;

    int result = sqlite3_exec(
        db,
        "ROLLBACK;",
        nullptr,
        nullptr,
        &errorMessage
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to roll back database transaction: "
             << (errorMessage != nullptr
                ? errorMessage
                : sqlite3_errmsg(db))
             << endl;
    }

    if (errorMessage != nullptr) {
        sqlite3_free(errorMessage);
    }
}

// Initializes the database connection and required schema
bool Database::initialize() {
    lock_guard<recursive_mutex> lock(databaseMutex);

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
    lock_guard<recursive_mutex> lock(databaseMutex);

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
    lock_guard<recursive_mutex> lock(databaseMutex);

    // All CREATE TABLE and CREATE INDEX statements are executed together
    // when the app starts. IF NOT EXISTS preserves existing user data.
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
        ");"

        // Stores reusable techniques created by individual users.
        // Technique names only need to be unique within the same user's
        // discipline.
        "CREATE TABLE IF NOT EXISTS combat_sports_techniques ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "discipline TEXT NOT NULL,"
        "name TEXT NOT NULL,"
        "category TEXT,"
        "description TEXT,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "UNIQUE (user_id, discipline, name),"
        "UNIQUE (user_id, id),"
        "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
        ");"

        // Stores user-created combinations. The individual ordered steps
        // are stored separately in combat_sports_combination_steps.
        "CREATE TABLE IF NOT EXISTS combat_sports_combinations ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "discipline TEXT NOT NULL,"
        "name TEXT NOT NULL,"
        "description TEXT,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "UNIQUE (user_id, discipline, name),"
        "UNIQUE (user_id, id),"
        "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
        ");"

        // Connects techniques to a combination in a defined order.
        // The composite foreign keys ensure that the combination and
        // technique both belong to the same authenticated user.
        "CREATE TABLE IF NOT EXISTS combat_sports_combination_steps ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "combination_id INTEGER NOT NULL,"
        "technique_id INTEGER NOT NULL,"
        "step_order INTEGER NOT NULL CHECK (step_order > 0),"
        "UNIQUE (combination_id, step_order),"
        "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,"
        "FOREIGN KEY (user_id, combination_id) "
        "REFERENCES combat_sports_combinations(user_id, id) "
        "ON DELETE CASCADE,"
        "FOREIGN KEY (user_id, technique_id) "
        "REFERENCES combat_sports_techniques(user_id, id) "
        "ON DELETE RESTRICT"
        ");"

        // Stores reusable user-created drills. Seconds are used for the
        // optional duration so trainer mode can support short intervals.
        "CREATE TABLE IF NOT EXISTS combat_sports_drills ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "discipline TEXT NOT NULL,"
        "name TEXT NOT NULL,"
        "instructions TEXT,"
        "default_duration_seconds INTEGER "
        "CHECK (default_duration_seconds IS NULL "
        "OR default_duration_seconds > 0),"
        "default_repetitions INTEGER "
        "CHECK (default_repetitions IS NULL "
        "OR default_repetitions > 0),"
        "default_rounds INTEGER "
        "CHECK (default_rounds IS NULL OR default_rounds > 0),"
        "notes TEXT,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "UNIQUE (user_id, discipline, name),"
        "UNIQUE (user_id, id),"
        "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
        ");"

        // Stores the ordered techniques and combinations contained in a
        // drill. Exactly one reference must be selected for each item.
        "CREATE TABLE IF NOT EXISTS combat_sports_drill_items ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "drill_id INTEGER NOT NULL,"
        "item_type TEXT NOT NULL "
        "CHECK (item_type IN ('technique', 'combination')),"
        "technique_id INTEGER,"
        "combination_id INTEGER,"
        "item_order INTEGER NOT NULL CHECK (item_order > 0),"
        "UNIQUE (drill_id, item_order),"

        // A technique item must reference only a technique, while a
        // combination item must reference only a combination.
        "CHECK ("
            "(item_type = 'technique' "
                "AND technique_id IS NOT NULL "
                "AND combination_id IS NULL)"
            " OR "
            "(item_type = 'combination' "
                "AND technique_id IS NULL "
                "AND combination_id IS NOT NULL)"
        "),"

        "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,"
        "FOREIGN KEY (user_id, drill_id) "
        "REFERENCES combat_sports_drills(user_id, id) "
        "ON DELETE CASCADE,"
        "FOREIGN KEY (user_id, technique_id) "
        "REFERENCES combat_sports_techniques(user_id, id) "
        "ON DELETE RESTRICT,"
        "FOREIGN KEY (user_id, combination_id) "
        "REFERENCES combat_sports_combinations(user_id, id) "
        "ON DELETE RESTRICT"
        ");"

        // Stores the reusable workout header. Disciplines, rounds, and
        // activities are stored separately so each list remains ordered and
        // can be replaced transactionally by later database operations.
        "CREATE TABLE IF NOT EXISTS combat_sports_workout_templates ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "name TEXT NOT NULL CHECK (length(trim(name)) > 0),"
        "description TEXT,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "UNIQUE (user_id, name),"
        "UNIQUE (user_id, id),"
        "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE"
        ");"

        // A template may represent one or several combat-sports disciplines.
        "CREATE TABLE IF NOT EXISTS combat_sports_workout_disciplines ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "workout_template_id INTEGER NOT NULL,"
        "discipline TEXT NOT NULL "
        "CHECK (length(trim(discipline)) > 0),"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "UNIQUE (workout_template_id, discipline),"
        "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,"
        "FOREIGN KEY (user_id, workout_template_id) "
        "REFERENCES combat_sports_workout_templates(user_id, id) "
        "ON DELETE CASCADE"
        ");"

        // Stores the ordered groups presented as rounds in Workout Builder.
        "CREATE TABLE IF NOT EXISTS combat_sports_workout_rounds ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "workout_template_id INTEGER NOT NULL,"
        "round_order INTEGER NOT NULL CHECK (round_order > 0),"
        "name TEXT,"
        "description TEXT,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "UNIQUE (workout_template_id, round_order),"
        "UNIQUE (user_id, id),"
        "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,"
        "FOREIGN KEY (user_id, workout_template_id) "
        "REFERENCES combat_sports_workout_templates(user_id, id) "
        "ON DELETE CASCADE"
        ");"

        // Stores one ordered planned activity. Referenced library content is
        // protected from deletion while a reusable template still needs it.
        "CREATE TABLE IF NOT EXISTS combat_sports_workout_activities ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "workout_template_id INTEGER NOT NULL,"
        "workout_round_id INTEGER NOT NULL,"
        "activity_order INTEGER NOT NULL CHECK (activity_order > 0),"
        "activity_type TEXT NOT NULL CHECK (activity_type IN ("
            "'technique', 'combination', 'drill', 'jump_rope', "
            "'conditioning', 'shadowboxing', 'rest', 'custom'"
        ")),"
        "technique_id INTEGER,"
        "combination_id INTEGER,"
        "drill_id INTEGER,"
        "name_snapshot TEXT NOT NULL "
        "CHECK (length(trim(name_snapshot)) > 0),"
        "instructions_snapshot TEXT,"
        "target_type TEXT NOT NULL CHECK (target_type IN ("
            "'repetitions', 'duration_seconds', 'rounds'"
        ")),"
        "target_value INTEGER NOT NULL CHECK (target_value > 0),"
        "target_sets INTEGER NOT NULL DEFAULT 1 CHECK (target_sets > 0),"
        "rest_after_seconds INTEGER NOT NULL DEFAULT 0 "
        "CHECK (rest_after_seconds >= 0),"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "UNIQUE (workout_round_id, activity_order),"
        "UNIQUE (user_id, id),"

        // Exactly one source is required for library activity types. Built-in,
        // rest, and custom activities intentionally have no library source.
        "CHECK ("
            "(activity_type = 'technique' "
                "AND technique_id IS NOT NULL "
                "AND combination_id IS NULL AND drill_id IS NULL)"
            " OR "
            "(activity_type = 'combination' "
                "AND technique_id IS NULL "
                "AND combination_id IS NOT NULL AND drill_id IS NULL)"
            " OR "
            "(activity_type = 'drill' "
                "AND technique_id IS NULL "
                "AND combination_id IS NULL AND drill_id IS NOT NULL)"
            " OR "
            "(activity_type IN ("
                "'jump_rope', 'conditioning', 'shadowboxing', "
                "'rest', 'custom'"
            ") AND technique_id IS NULL "
                "AND combination_id IS NULL AND drill_id IS NULL)"
        "),"

        "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,"
        "FOREIGN KEY (user_id, workout_template_id) "
        "REFERENCES combat_sports_workout_templates(user_id, id) "
        "ON DELETE CASCADE,"
        "FOREIGN KEY (user_id, workout_round_id) "
        "REFERENCES combat_sports_workout_rounds(user_id, id) "
        "ON DELETE CASCADE,"
        "FOREIGN KEY (user_id, technique_id) "
        "REFERENCES combat_sports_techniques(user_id, id) "
        "ON DELETE RESTRICT,"
        "FOREIGN KEY (user_id, combination_id) "
        "REFERENCES combat_sports_combinations(user_id, id) "
        "ON DELETE RESTRICT,"
        "FOREIGN KEY (user_id, drill_id) "
        "REFERENCES combat_sports_drills(user_id, id) "
        "ON DELETE RESTRICT"
        ");"

        // Stores the confirmed result shared by Training Mode and manual
        // recording. The linked session remains the main history entry.
        "CREATE TABLE IF NOT EXISTS combat_sports_completed_workouts ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "workout_template_id INTEGER,"
        "combat_sports_session_id INTEGER NOT NULL UNIQUE,"
        "workout_name_snapshot TEXT NOT NULL "
        "CHECK (length(trim(workout_name_snapshot)) > 0),"
        "workout_description_snapshot TEXT,"
        "recording_method TEXT NOT NULL "
        "CHECK (recording_method IN ('training_mode', 'manual')),"
        "started_at TEXT,"
        "completed_at TEXT,"
        "planned_duration_seconds INTEGER "
        "CHECK (planned_duration_seconds IS NULL "
        "OR planned_duration_seconds > 0),"
        "actual_duration_seconds INTEGER NOT NULL "
        "CHECK (actual_duration_seconds > 0),"
        "stopped_early INTEGER NOT NULL DEFAULT 0 "
        "CHECK (stopped_early IN (0, 1)),"
        "notes TEXT,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "UNIQUE (user_id, id),"
        "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,"
        "FOREIGN KEY (workout_template_id) "
        "REFERENCES combat_sports_workout_templates(id) "
        "ON DELETE SET NULL,"
        "FOREIGN KEY (combat_sports_session_id) "
        "REFERENCES combat_sports_sessions(id) ON DELETE CASCADE"
        ");"

        // Completed rounds are snapshots and therefore survive deletion of
        // their optional source template round.
        "CREATE TABLE IF NOT EXISTS combat_sports_completed_workout_rounds ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "completed_workout_id INTEGER NOT NULL,"
        "source_workout_round_id INTEGER,"
        "round_order INTEGER NOT NULL CHECK (round_order > 0),"
        "name_snapshot TEXT,"
        "description_snapshot TEXT,"
        "status TEXT NOT NULL "
        "CHECK (status IN ('completed', 'partial', 'skipped')),"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "UNIQUE (completed_workout_id, round_order),"
        "UNIQUE (user_id, id),"
        "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,"
        "FOREIGN KEY (user_id, completed_workout_id) "
        "REFERENCES combat_sports_completed_workouts(user_id, id) "
        "ON DELETE CASCADE,"
        "FOREIGN KEY (source_workout_round_id) "
        "REFERENCES combat_sports_workout_rounds(id) ON DELETE SET NULL"
        ");"

        // Stores actual activity results. Nullable source references plus
        // snapshots preserve completed history after template/library edits.
        "CREATE TABLE IF NOT EXISTS "
        "combat_sports_completed_workout_activities ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "completed_workout_id INTEGER NOT NULL,"
        "completed_workout_round_id INTEGER NOT NULL,"
        "source_workout_activity_id INTEGER,"
        "activity_order INTEGER NOT NULL CHECK (activity_order > 0),"
        "activity_type TEXT NOT NULL CHECK (activity_type IN ("
            "'technique', 'combination', 'drill', 'jump_rope', "
            "'conditioning', 'shadowboxing', 'rest', 'custom'"
        ")),"
        "technique_id INTEGER,"
        "combination_id INTEGER,"
        "drill_id INTEGER,"
        "name_snapshot TEXT NOT NULL "
        "CHECK (length(trim(name_snapshot)) > 0),"
        "instructions_snapshot TEXT,"
        "target_type TEXT NOT NULL CHECK (target_type IN ("
            "'repetitions', 'duration_seconds', 'rounds'"
        ")),"
        "planned_value INTEGER CHECK (planned_value IS NULL "
        "OR planned_value > 0),"
        "planned_sets INTEGER CHECK (planned_sets IS NULL "
        "OR planned_sets > 0),"
        "completed_value INTEGER NOT NULL DEFAULT 0 "
        "CHECK (completed_value >= 0),"
        "completed_sets INTEGER NOT NULL DEFAULT 0 "
        "CHECK (completed_sets >= 0),"
        "actual_duration_seconds INTEGER "
        "CHECK (actual_duration_seconds IS NULL "
        "OR actual_duration_seconds > 0),"
        "status TEXT NOT NULL "
        "CHECK (status IN ('completed', 'partial', 'skipped')),"
        "was_unplanned INTEGER NOT NULL DEFAULT 0 "
        "CHECK (was_unplanned IN (0, 1)),"
        "notes TEXT,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "UNIQUE (completed_workout_round_id, activity_order),"
        "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,"
        "FOREIGN KEY (user_id, completed_workout_id) "
        "REFERENCES combat_sports_completed_workouts(user_id, id) "
        "ON DELETE CASCADE,"
        "FOREIGN KEY (user_id, completed_workout_round_id) "
        "REFERENCES combat_sports_completed_workout_rounds(user_id, id) "
        "ON DELETE CASCADE,"
        "FOREIGN KEY (source_workout_activity_id) "
        "REFERENCES combat_sports_workout_activities(id) "
        "ON DELETE SET NULL,"
        "FOREIGN KEY (technique_id) "
        "REFERENCES combat_sports_techniques(id) ON DELETE SET NULL,"
        "FOREIGN KEY (combination_id) "
        "REFERENCES combat_sports_combinations(id) ON DELETE SET NULL,"
        "FOREIGN KEY (drill_id) "
        "REFERENCES combat_sports_drills(id) ON DELETE SET NULL"
        ");"

        // Stores calculated individual technique volume. Technique snapshots
        // keep historical totals readable when library content changes.
        "CREATE TABLE IF NOT EXISTS combat_sports_completed_technique_totals ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "completed_workout_id INTEGER NOT NULL,"
        "technique_id INTEGER,"
        "technique_name_snapshot TEXT NOT NULL "
        "CHECK (length(trim(technique_name_snapshot)) > 0),"
        "technique_category_snapshot TEXT,"
        "total_repetitions INTEGER NOT NULL CHECK (total_repetitions > 0),"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "UNIQUE (completed_workout_id, technique_name_snapshot),"
        "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,"
        "FOREIGN KEY (user_id, completed_workout_id) "
        "REFERENCES combat_sports_completed_workouts(user_id, id) "
        "ON DELETE CASCADE,"
        "FOREIGN KEY (technique_id) "
        "REFERENCES combat_sports_techniques(id) ON DELETE SET NULL"
        ");"

        // These indexes support the user-scoped list operations that will
        // be implemented during Issue 2.2.
        "CREATE INDEX IF NOT EXISTS "
        "idx_combat_sports_techniques_user "
        "ON combat_sports_techniques(user_id);"

        "CREATE INDEX IF NOT EXISTS "
        "idx_combat_sports_combinations_user "
        "ON combat_sports_combinations(user_id);"

        "CREATE INDEX IF NOT EXISTS "
        "idx_combat_sports_combination_steps_combination "
        "ON combat_sports_combination_steps(combination_id, step_order);"

        "CREATE INDEX IF NOT EXISTS "
        "idx_combat_sports_drills_user "
        "ON combat_sports_drills(user_id);"

        "CREATE INDEX IF NOT EXISTS "
        "idx_combat_sports_drill_items_drill "
        "ON combat_sports_drill_items(drill_id, item_order);"

        // Workout Builder indexes keep user lists and ordered child lookups
        // efficient as templates grow.
        "CREATE INDEX IF NOT EXISTS "
        "idx_combat_sports_workout_templates_user "
        "ON combat_sports_workout_templates(user_id);"

        "CREATE INDEX IF NOT EXISTS "
        "idx_combat_sports_workout_disciplines_template "
        "ON combat_sports_workout_disciplines(workout_template_id);"

        "CREATE INDEX IF NOT EXISTS "
        "idx_combat_sports_workout_disciplines_user_discipline "
        "ON combat_sports_workout_disciplines(user_id, discipline);"

        "CREATE INDEX IF NOT EXISTS "
        "idx_combat_sports_workout_rounds_template "
        "ON combat_sports_workout_rounds("
        "workout_template_id, round_order);"

        "CREATE INDEX IF NOT EXISTS "
        "idx_combat_sports_workout_activities_round "
        "ON combat_sports_workout_activities("
        "workout_round_id, activity_order);"

        "CREATE INDEX IF NOT EXISTS "
        "idx_combat_sports_workout_activities_technique "
        "ON combat_sports_workout_activities(technique_id);"

        "CREATE INDEX IF NOT EXISTS "
        "idx_combat_sports_workout_activities_combination "
        "ON combat_sports_workout_activities(combination_id);"

        "CREATE INDEX IF NOT EXISTS "
        "idx_combat_sports_workout_activities_drill "
        "ON combat_sports_workout_activities(drill_id);"

        // Completed-workout indexes support history, template comparisons,
        // and technique-volume summaries.
        "CREATE INDEX IF NOT EXISTS "
        "idx_combat_sports_completed_workouts_user "
        "ON combat_sports_completed_workouts(user_id);"

        "CREATE INDEX IF NOT EXISTS "
        "idx_combat_sports_completed_workouts_template "
        "ON combat_sports_completed_workouts(workout_template_id);"

        "CREATE INDEX IF NOT EXISTS "
        "idx_combat_sports_completed_workouts_recording_method "
        "ON combat_sports_completed_workouts(user_id, recording_method);"

        "CREATE INDEX IF NOT EXISTS "
        "idx_combat_sports_completed_rounds_workout "
        "ON combat_sports_completed_workout_rounds("
        "completed_workout_id, round_order);"

        "CREATE INDEX IF NOT EXISTS "
        "idx_combat_sports_completed_activities_round "
        "ON combat_sports_completed_workout_activities("
        "completed_workout_round_id, activity_order);"

        "CREATE INDEX IF NOT EXISTS "
        "idx_combat_sports_completed_activities_workout "
        "ON combat_sports_completed_workout_activities("
        "completed_workout_id);"

        "CREATE INDEX IF NOT EXISTS "
        "idx_combat_sports_completed_totals_workout "
        "ON combat_sports_completed_technique_totals("
        "completed_workout_id);"

        "CREATE INDEX IF NOT EXISTS "
        "idx_combat_sports_completed_totals_user_technique "
        "ON combat_sports_completed_technique_totals("
        "user_id, technique_id);";

    // SQLite stores any schema error message in this pointer.
    char* errorMessage = nullptr;

    // Execute all schema statements together.
    int result = sqlite3_exec(
        db,
        sql,
        nullptr,
        nullptr,
        &errorMessage
    );

    // Stop initialization if any table or index could not be created.
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
    lock_guard<recursive_mutex> lock(databaseMutex);

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
    lock_guard<recursive_mutex> lock(databaseMutex);

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
    lock_guard<recursive_mutex> lock(databaseMutex);

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
    lock_guard<recursive_mutex> lock(databaseMutex);

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
    lock_guard<recursive_mutex> lock(databaseMutex);

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
    lock_guard<recursive_mutex> lock(databaseMutex);

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
    lock_guard<recursive_mutex> lock(databaseMutex);

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
    lock_guard<recursive_mutex> lock(databaseMutex);

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
    lock_guard<recursive_mutex> lock(databaseMutex);

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
    lock_guard<recursive_mutex> lock(databaseMutex);

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

// Creates a reusable combat-sports technique for a specific user
DatabaseResult Database::createCombatSportsTechnique(
    int userId,
    const string& discipline,
    const string& name,
    const string& category,
    const string& description,
    int& techniqueId
) {
    lock_guard<recursive_mutex> lock(databaseMutex);

    const char* sql =
        "INSERT INTO combat_sports_techniques ("
        "user_id, discipline, name, category, description"
        ") VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt* statement = nullptr;

    // Convert the INSERT command into a prepared statement.
    int result = sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare combat sports technique insert statement: "
             << sqlite3_errmsg(db) << endl;

        return DatabaseResult::Error;
    }

    // Associate the new technique with its owning user.
    result = sqlite3_bind_int(
        statement,
        1,
        userId
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to bind combat sports technique user ID: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    // Bind the technique information to the remaining placeholders.
    if (!bindText(statement, 2, discipline) ||
        !bindText(statement, 3, name) ||
        !bindText(statement, 4, category) ||
        !bindText(statement, 5, description)) {

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    // Execute the prepared INSERT statement.
    result = sqlite3_step(statement);

    sqlite3_finalize(statement);

    if (result == SQLITE_DONE) {
        // Save the generated ID so the caller can retrieve or return
        // the newly created technique.
        techniqueId = static_cast<int>(
            sqlite3_last_insert_rowid(db)
        );

        return DatabaseResult::Success;
    }

    // This includes duplicate names within the same user and discipline,
    // as well as invalid user foreign-key references.
    if (result == SQLITE_CONSTRAINT) {
        cerr << "Combat sports technique creation conflict: "
             << sqlite3_errmsg(db) << endl;

        return DatabaseResult::Conflict;
    }

    cerr << "Failed to create combat sports technique: "
         << sqlite3_errmsg(db) << endl;

    return DatabaseResult::Error;
}

// Retrieves one technique belonging to a specific user
DatabaseResult Database::getCombatSportsTechnique(
    int techniqueId,
    int userId,
    CombatSportsTechnique& technique
) {
    lock_guard<recursive_mutex> lock(databaseMutex);

    const char* sql =
        "SELECT "
        "id, user_id, discipline, name, category, description, "
        "created_at, updated_at "
        "FROM combat_sports_techniques "
        "WHERE id = ? AND user_id = ?;";

    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare combat sports technique lookup statement: "
             << sqlite3_errmsg(db) << endl;

        return DatabaseResult::Error;
    }

    // Bind both IDs so a user cannot retrieve another user's technique.
    result = sqlite3_bind_int(
        statement,
        1,
        techniqueId
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to bind combat sports technique ID: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    result = sqlite3_bind_int(
        statement,
        2,
        userId
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to bind combat sports technique owner ID: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    result = sqlite3_step(statement);

    if (result == SQLITE_ROW) {
        technique.id = sqlite3_column_int(statement, 0);
        technique.userId = sqlite3_column_int(statement, 1);

        technique.discipline = reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 2)
        );

        technique.name = reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 3)
        );

        // Category and description are optional database columns.
        const unsigned char* storedCategory =
            sqlite3_column_text(statement, 4);

        const unsigned char* storedDescription =
            sqlite3_column_text(statement, 5);

        technique.category = storedCategory != nullptr
            ? reinterpret_cast<const char*>(storedCategory)
            : "";

        technique.description = storedDescription != nullptr
            ? reinterpret_cast<const char*>(storedDescription)
            : "";

        technique.createdAt = reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 6)
        );

        technique.updatedAt = reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 7)
        );

        sqlite3_finalize(statement);

        return DatabaseResult::Success;
    }

    if (result == SQLITE_DONE) {
        sqlite3_finalize(statement);

        return DatabaseResult::NotFound;
    }

    cerr << "Failed to retrieve combat sports technique: "
         << sqlite3_errmsg(db) << endl;

    sqlite3_finalize(statement);

    return DatabaseResult::Error;
}

// Retrieves all techniques belonging to a specific user
DatabaseResult Database::getCombatSportsTechniques(
    int userId,
    vector<CombatSportsTechnique>& techniques
) {
    lock_guard<recursive_mutex> lock(databaseMutex);

    const char* sql =
        "SELECT "
        "id, user_id, discipline, name, category, description, "
        "created_at, updated_at "
        "FROM combat_sports_techniques "
        "WHERE user_id = ? "
        "ORDER BY discipline ASC, name ASC, id ASC;";

    sqlite3_stmt* statement = nullptr;

    // Prevent older results from remaining in the output vector.
    techniques.clear();

    int result = sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare combat sports techniques lookup statement: "
             << sqlite3_errmsg(db) << endl;

        return DatabaseResult::Error;
    }

    result = sqlite3_bind_int(
        statement,
        1,
        userId
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to bind combat sports techniques owner ID: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    // Read each matching technique until the query is complete.
    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        CombatSportsTechnique technique;

        technique.id = sqlite3_column_int(statement, 0);
        technique.userId = sqlite3_column_int(statement, 1);

        technique.discipline = reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 2)
        );

        technique.name = reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 3)
        );

        const unsigned char* storedCategory =
            sqlite3_column_text(statement, 4);

        const unsigned char* storedDescription =
            sqlite3_column_text(statement, 5);

        technique.category = storedCategory != nullptr
            ? reinterpret_cast<const char*>(storedCategory)
            : "";

        technique.description = storedDescription != nullptr
            ? reinterpret_cast<const char*>(storedDescription)
            : "";

        technique.createdAt = reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 6)
        );

        technique.updatedAt = reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 7)
        );

        techniques.push_back(technique);
    }

    if (result == SQLITE_DONE) {
        sqlite3_finalize(statement);

        // An empty vector is still a successful retrieval.
        return DatabaseResult::Success;
    }

    cerr << "Failed to retrieve combat sports techniques: "
         << sqlite3_errmsg(db) << endl;

    sqlite3_finalize(statement);

    // Do not expose partially loaded results after a database error.
    techniques.clear();

    return DatabaseResult::Error;
}

// Updates a technique belonging to a specific user
DatabaseResult Database::updateCombatSportsTechnique(
    int techniqueId,
    int userId,
    const string& discipline,
    const string& name,
    const string& category,
    const string& description
) {
    lock_guard<recursive_mutex> lock(databaseMutex);

    const char* sql =
        "UPDATE combat_sports_techniques "
        "SET discipline = ?, "
        "name = ?, "
        "category = ?, "
        "description = ?, "
        "updated_at = CURRENT_TIMESTAMP "
        "WHERE id = ? AND user_id = ?;";

    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare combat sports technique update statement: "
             << sqlite3_errmsg(db) << endl;

        return DatabaseResult::Error;
    }

    // Bind the replacement technique values.
    if (!bindText(statement, 1, discipline) ||
        !bindText(statement, 2, name) ||
        !bindText(statement, 3, category) ||
        !bindText(statement, 4, description)) {

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    // Restrict the update to the requested technique and its owner.
    result = sqlite3_bind_int(
        statement,
        5,
        techniqueId
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to bind combat sports technique ID: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    result = sqlite3_bind_int(
        statement,
        6,
        userId
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to bind combat sports technique owner ID: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    result = sqlite3_step(statement);

    if (result == SQLITE_DONE) {
        int updatedRows = sqlite3_changes(db);

        sqlite3_finalize(statement);

        if (updatedRows == 0) {
            return DatabaseResult::NotFound;
        }

        return DatabaseResult::Success;
    }

    // Duplicate names and other constraint failures are reported as
    // conflicts instead of unexpected database errors.
    if (result == SQLITE_CONSTRAINT) {
        cerr << "Combat sports technique update conflict: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Conflict;
    }

    cerr << "Failed to update combat sports technique: "
         << sqlite3_errmsg(db) << endl;

    sqlite3_finalize(statement);

    return DatabaseResult::Error;
}

// Deletes a technique belonging to a specific user
DatabaseResult Database::deleteCombatSportsTechnique(
    int techniqueId,
    int userId
) {
    lock_guard<recursive_mutex> lock(databaseMutex);

    const char* sql =
        "DELETE FROM combat_sports_techniques "
        "WHERE id = ? AND user_id = ?;";

    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare combat sports technique delete statement: "
             << sqlite3_errmsg(db) << endl;

        return DatabaseResult::Error;
    }

    result = sqlite3_bind_int(
        statement,
        1,
        techniqueId
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to bind combat sports technique ID: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    // Including the user ID prevents one user from deleting another
    // user's technique.
    result = sqlite3_bind_int(
        statement,
        2,
        userId
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to bind combat sports technique owner ID: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    result = sqlite3_step(statement);

    if (result == SQLITE_DONE) {
        int deletedRows = sqlite3_changes(db);

        sqlite3_finalize(statement);

        if (deletedRows == 0) {
            return DatabaseResult::NotFound;
        }

        return DatabaseResult::Success;
    }

    // A referenced technique is protected by the schema's
    // ON DELETE RESTRICT foreign-key rules.
    if (result == SQLITE_CONSTRAINT) {
        cerr << "Combat sports technique deletion conflict: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Conflict;
    }

    cerr << "Failed to delete combat sports technique: "
         << sqlite3_errmsg(db) << endl;

    sqlite3_finalize(statement);

    return DatabaseResult::Error;
}

// Creates a combination and its ordered technique steps as one transaction
DatabaseResult Database::createCombatSportsCombination(
    int userId,
    const string& discipline,
    const string& name,
    const string& description,
    const vector<int>& techniqueIds,
    int& combinationId
) {
    lock_guard<recursive_mutex> lock(databaseMutex);

    // A combination must contain at least one technique.
    if (techniqueIds.empty()) {
        cerr << "Cannot create an empty combat sports combination."
             << endl;

        return DatabaseResult::Conflict;
    }

    if (!beginTransaction()) {
        return DatabaseResult::Error;
    }

    const char* combinationSql =
        "INSERT INTO combat_sports_combinations ("
        "user_id, discipline, name, description"
        ") VALUES (?, ?, ?, ?);";

    sqlite3_stmt* combinationStatement = nullptr;

    int result = sqlite3_prepare_v2(
        db,
        combinationSql,
        -1,
        &combinationStatement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare combat sports combination insert: "
             << sqlite3_errmsg(db) << endl;

        rollbackTransaction();
        return DatabaseResult::Error;
    }

    result = sqlite3_bind_int(
        combinationStatement,
        1,
        userId
    );

    if (result != SQLITE_OK ||
        !bindText(combinationStatement, 2, discipline) ||
        !bindText(combinationStatement, 3, name) ||
        !bindText(combinationStatement, 4, description)) {

        sqlite3_finalize(combinationStatement);
        rollbackTransaction();

        return DatabaseResult::Error;
    }

    result = sqlite3_step(combinationStatement);

    sqlite3_finalize(combinationStatement);

    if (result != SQLITE_DONE) {
        DatabaseResult operationResult =
            result == SQLITE_CONSTRAINT
                ? DatabaseResult::Conflict
                : DatabaseResult::Error;

        cerr << "Failed to create combat sports combination: "
             << sqlite3_errmsg(db) << endl;

        rollbackTransaction();

        return operationResult;
    }

    int newCombinationId = static_cast<int>(
        sqlite3_last_insert_rowid(db)
    );

    // INSERT...SELECT only creates a step when the referenced technique
    // belongs to the same user and discipline as the combination.
    const char* stepSql =
        "INSERT INTO combat_sports_combination_steps ("
        "user_id, combination_id, technique_id, step_order"
        ") "
        "SELECT ?, ?, id, ? "
        "FROM combat_sports_techniques "
        "WHERE id = ? "
        "AND user_id = ? "
        "AND discipline = ?;";

    sqlite3_stmt* stepStatement = nullptr;

    result = sqlite3_prepare_v2(
        db,
        stepSql,
        -1,
        &stepStatement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare combination step insert: "
             << sqlite3_errmsg(db) << endl;

        rollbackTransaction();
        return DatabaseResult::Error;
    }

    for (size_t index = 0; index < techniqueIds.size(); index++) {
        // Reuse the same prepared statement for every ordered step.
        sqlite3_reset(stepStatement);
        sqlite3_clear_bindings(stepStatement);

        int stepOrder = static_cast<int>(index) + 1;

        result = sqlite3_bind_int(
            stepStatement,
            1,
            userId
        );

        if (result == SQLITE_OK) {
            result = sqlite3_bind_int(
                stepStatement,
                2,
                newCombinationId
            );
        }

        if (result == SQLITE_OK) {
            result = sqlite3_bind_int(
                stepStatement,
                3,
                stepOrder
            );
        }

        if (result == SQLITE_OK) {
            result = sqlite3_bind_int(
                stepStatement,
                4,
                techniqueIds[index]
            );
        }

        if (result == SQLITE_OK) {
            result = sqlite3_bind_int(
                stepStatement,
                5,
                userId
            );
        }

        if (result != SQLITE_OK ||
            !bindText(stepStatement, 6, discipline)) {

            cerr << "Failed to bind combination step values: "
                 << sqlite3_errmsg(db) << endl;

            sqlite3_finalize(stepStatement);
            rollbackTransaction();

            return DatabaseResult::Error;
        }

        result = sqlite3_step(stepStatement);

        if (result != SQLITE_DONE) {
            DatabaseResult operationResult =
                result == SQLITE_CONSTRAINT
                    ? DatabaseResult::Conflict
                    : DatabaseResult::Error;

            cerr << "Failed to insert combination step: "
                 << sqlite3_errmsg(db) << endl;

            sqlite3_finalize(stepStatement);
            rollbackTransaction();

            return operationResult;
        }

        // Zero changes means INSERT...SELECT could not find a technique
        // owned by this user with the required discipline.
        if (sqlite3_changes(db) == 0) {
            cerr << "Combination technique was not found or did not "
                 << "match the combination discipline."
                 << endl;

            sqlite3_finalize(stepStatement);
            rollbackTransaction();

            return DatabaseResult::NotFound;
        }
    }

    sqlite3_finalize(stepStatement);

    if (!commitTransaction()) {
        rollbackTransaction();
        return DatabaseResult::Error;
    }

    // Only expose the generated ID after every step has committed.
    combinationId = newCombinationId;

    return DatabaseResult::Success;
}

// Retrieves one combination and its ordered technique steps
DatabaseResult Database::getCombatSportsCombination(
    int combinationId,
    int userId,
    CombatSportsCombination& combination
) {
    lock_guard<recursive_mutex> lock(databaseMutex);

    const char* combinationSql =
        "SELECT "
        "id, user_id, discipline, name, description, "
        "created_at, updated_at "
        "FROM combat_sports_combinations "
        "WHERE id = ? AND user_id = ?;";

    sqlite3_stmt* combinationStatement = nullptr;

    int result = sqlite3_prepare_v2(
        db,
        combinationSql,
        -1,
        &combinationStatement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare combination lookup statement: "
             << sqlite3_errmsg(db) << endl;

        return DatabaseResult::Error;
    }

    result = sqlite3_bind_int(
        combinationStatement,
        1,
        combinationId
    );

    if (result == SQLITE_OK) {
        result = sqlite3_bind_int(
            combinationStatement,
            2,
            userId
        );
    }

    if (result != SQLITE_OK) {
        cerr << "Failed to bind combination lookup values: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(combinationStatement);
        return DatabaseResult::Error;
    }

    result = sqlite3_step(combinationStatement);

    if (result == SQLITE_DONE) {
        sqlite3_finalize(combinationStatement);
        return DatabaseResult::NotFound;
    }

    if (result != SQLITE_ROW) {
        cerr << "Failed to retrieve combat sports combination: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(combinationStatement);
        return DatabaseResult::Error;
    }

    combination.id = sqlite3_column_int(
        combinationStatement,
        0
    );

    combination.userId = sqlite3_column_int(
        combinationStatement,
        1
    );

    combination.discipline = reinterpret_cast<const char*>(
        sqlite3_column_text(combinationStatement, 2)
    );

    combination.name = reinterpret_cast<const char*>(
        sqlite3_column_text(combinationStatement, 3)
    );

    const unsigned char* storedDescription =
        sqlite3_column_text(combinationStatement, 4);

    combination.description = storedDescription != nullptr
        ? reinterpret_cast<const char*>(storedDescription)
        : "";

    combination.createdAt = reinterpret_cast<const char*>(
        sqlite3_column_text(combinationStatement, 5)
    );

    combination.updatedAt = reinterpret_cast<const char*>(
        sqlite3_column_text(combinationStatement, 6)
    );

    combination.steps.clear();

    sqlite3_finalize(combinationStatement);

    const char* stepsSql =
        "SELECT "
        "steps.id, steps.user_id, steps.combination_id, "
        "steps.technique_id, steps.step_order, "
        "techniques.name, techniques.category "
        "FROM combat_sports_combination_steps AS steps "
        "INNER JOIN combat_sports_techniques AS techniques "
        "ON techniques.id = steps.technique_id "
        "AND techniques.user_id = steps.user_id "
        "WHERE steps.combination_id = ? "
        "AND steps.user_id = ? "
        "ORDER BY steps.step_order ASC;";

    sqlite3_stmt* stepsStatement = nullptr;

    result = sqlite3_prepare_v2(
        db,
        stepsSql,
        -1,
        &stepsStatement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare combination steps lookup: "
             << sqlite3_errmsg(db) << endl;

        return DatabaseResult::Error;
    }

    result = sqlite3_bind_int(
        stepsStatement,
        1,
        combinationId
    );

    if (result == SQLITE_OK) {
        result = sqlite3_bind_int(
            stepsStatement,
            2,
            userId
        );
    }

    if (result != SQLITE_OK) {
        cerr << "Failed to bind combination steps lookup values: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(stepsStatement);
        return DatabaseResult::Error;
    }

    while ((result = sqlite3_step(stepsStatement)) == SQLITE_ROW) {
        CombatSportsCombinationStep step;

        step.id = sqlite3_column_int(stepsStatement, 0);
        step.userId = sqlite3_column_int(stepsStatement, 1);
        step.combinationId = sqlite3_column_int(stepsStatement, 2);
        step.techniqueId = sqlite3_column_int(stepsStatement, 3);
        step.stepOrder = sqlite3_column_int(stepsStatement, 4);

        step.techniqueName = reinterpret_cast<const char*>(
            sqlite3_column_text(stepsStatement, 5)
        );

        const unsigned char* storedCategory =
            sqlite3_column_text(stepsStatement, 6);

        step.techniqueCategory = storedCategory != nullptr
            ? reinterpret_cast<const char*>(storedCategory)
            : "";

        combination.steps.push_back(step);
    }

    if (result == SQLITE_DONE) {
        sqlite3_finalize(stepsStatement);
        return DatabaseResult::Success;
    }

    cerr << "Failed to retrieve combination steps: "
         << sqlite3_errmsg(db) << endl;

    sqlite3_finalize(stepsStatement);
    combination.steps.clear();

    return DatabaseResult::Error;
}

// Retrieves all combinations belonging to a specific user
DatabaseResult Database::getCombatSportsCombinations(
    int userId,
    vector<CombatSportsCombination>& combinations
) {
    lock_guard<recursive_mutex> lock(databaseMutex);

    const char* sql =
        "SELECT id "
        "FROM combat_sports_combinations "
        "WHERE user_id = ? "
        "ORDER BY discipline ASC, name ASC, id ASC;";

    sqlite3_stmt* statement = nullptr;
    vector<int> combinationIds;

    combinations.clear();

    int result = sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare combinations lookup statement: "
             << sqlite3_errmsg(db) << endl;

        return DatabaseResult::Error;
    }

    result = sqlite3_bind_int(
        statement,
        1,
        userId
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to bind combinations owner ID: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    // Collect the IDs first and finalize this statement before running
    // the detailed lookup for each combination.
    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        combinationIds.push_back(
            sqlite3_column_int(statement, 0)
        );
    }

    if (result != SQLITE_DONE) {
        cerr << "Failed to retrieve combination IDs: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    sqlite3_finalize(statement);

    for (int storedCombinationId : combinationIds) {
        CombatSportsCombination combination;

        DatabaseResult lookupResult =
            getCombatSportsCombination(
                storedCombinationId,
                userId,
                combination
            );

        if (lookupResult != DatabaseResult::Success) {
            combinations.clear();
            return DatabaseResult::Error;
        }

        combinations.push_back(combination);
    }

    // An empty vector is a successful retrieval when the user has not
    // created any combinations yet.
    return DatabaseResult::Success;
}

// Updates a combination and completely replaces its ordered steps
DatabaseResult Database::updateCombatSportsCombination(
    int combinationId,
    int userId,
    const string& discipline,
    const string& name,
    const string& description,
    const vector<int>& techniqueIds
) {
    lock_guard<recursive_mutex> lock(databaseMutex);

    if (techniqueIds.empty()) {
        cerr << "Cannot update a combination with no techniques."
             << endl;

        return DatabaseResult::Conflict;
    }

    if (!beginTransaction()) {
        return DatabaseResult::Error;
    }

    const char* updateSql =
        "UPDATE combat_sports_combinations "
        "SET discipline = ?, "
        "name = ?, "
        "description = ?, "
        "updated_at = CURRENT_TIMESTAMP "
        "WHERE id = ? AND user_id = ?;";

    sqlite3_stmt* updateStatement = nullptr;

    int result = sqlite3_prepare_v2(
        db,
        updateSql,
        -1,
        &updateStatement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare combination update statement: "
             << sqlite3_errmsg(db) << endl;

        rollbackTransaction();
        return DatabaseResult::Error;
    }

    if (!bindText(updateStatement, 1, discipline) ||
        !bindText(updateStatement, 2, name) ||
        !bindText(updateStatement, 3, description)) {

        sqlite3_finalize(updateStatement);
        rollbackTransaction();

        return DatabaseResult::Error;
    }

    result = sqlite3_bind_int(
        updateStatement,
        4,
        combinationId
    );

    if (result == SQLITE_OK) {
        result = sqlite3_bind_int(
            updateStatement,
            5,
            userId
        );
    }

    if (result != SQLITE_OK) {
        cerr << "Failed to bind combination update identifiers: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(updateStatement);
        rollbackTransaction();

        return DatabaseResult::Error;
    }

    result = sqlite3_step(updateStatement);

    if (result != SQLITE_DONE) {
        DatabaseResult operationResult =
            result == SQLITE_CONSTRAINT
                ? DatabaseResult::Conflict
                : DatabaseResult::Error;

        cerr << "Failed to update combat sports combination: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(updateStatement);
        rollbackTransaction();

        return operationResult;
    }

    int updatedRows = sqlite3_changes(db);

    sqlite3_finalize(updateStatement);

    if (updatedRows == 0) {
        rollbackTransaction();
        return DatabaseResult::NotFound;
    }

    // Existing steps are removed only inside the transaction. If any new
    // step fails, rollback restores the original combination and steps.
    const char* deleteStepsSql =
        "DELETE FROM combat_sports_combination_steps "
        "WHERE combination_id = ? AND user_id = ?;";

    sqlite3_stmt* deleteStepsStatement = nullptr;

    result = sqlite3_prepare_v2(
        db,
        deleteStepsSql,
        -1,
        &deleteStepsStatement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare old combination steps deletion: "
             << sqlite3_errmsg(db) << endl;

        rollbackTransaction();
        return DatabaseResult::Error;
    }

    result = sqlite3_bind_int(
        deleteStepsStatement,
        1,
        combinationId
    );

    if (result == SQLITE_OK) {
        result = sqlite3_bind_int(
            deleteStepsStatement,
            2,
            userId
        );
    }

    if (result != SQLITE_OK) {
        cerr << "Failed to bind old combination step identifiers: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(deleteStepsStatement);
        rollbackTransaction();

        return DatabaseResult::Error;
    }

    result = sqlite3_step(deleteStepsStatement);

    sqlite3_finalize(deleteStepsStatement);

    if (result != SQLITE_DONE) {
        cerr << "Failed to delete old combination steps: "
             << sqlite3_errmsg(db) << endl;

        rollbackTransaction();
        return DatabaseResult::Error;
    }

    const char* insertStepSql =
        "INSERT INTO combat_sports_combination_steps ("
        "user_id, combination_id, technique_id, step_order"
        ") "
        "SELECT ?, ?, id, ? "
        "FROM combat_sports_techniques "
        "WHERE id = ? "
        "AND user_id = ? "
        "AND discipline = ?;";

    sqlite3_stmt* insertStepStatement = nullptr;

    result = sqlite3_prepare_v2(
        db,
        insertStepSql,
        -1,
        &insertStepStatement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare updated combination step insert: "
             << sqlite3_errmsg(db) << endl;

        rollbackTransaction();
        return DatabaseResult::Error;
    }

    for (size_t index = 0; index < techniqueIds.size(); index++) {
        sqlite3_reset(insertStepStatement);
        sqlite3_clear_bindings(insertStepStatement);

        int stepOrder = static_cast<int>(index) + 1;

        result = sqlite3_bind_int(
            insertStepStatement,
            1,
            userId
        );

        if (result == SQLITE_OK) {
            result = sqlite3_bind_int(
                insertStepStatement,
                2,
                combinationId
            );
        }

        if (result == SQLITE_OK) {
            result = sqlite3_bind_int(
                insertStepStatement,
                3,
                stepOrder
            );
        }

        if (result == SQLITE_OK) {
            result = sqlite3_bind_int(
                insertStepStatement,
                4,
                techniqueIds[index]
            );
        }

        if (result == SQLITE_OK) {
            result = sqlite3_bind_int(
                insertStepStatement,
                5,
                userId
            );
        }

        if (result != SQLITE_OK ||
            !bindText(insertStepStatement, 6, discipline)) {

            cerr << "Failed to bind updated combination step: "
                 << sqlite3_errmsg(db) << endl;

            sqlite3_finalize(insertStepStatement);
            rollbackTransaction();

            return DatabaseResult::Error;
        }

        result = sqlite3_step(insertStepStatement);

        if (result != SQLITE_DONE) {
            DatabaseResult operationResult =
                result == SQLITE_CONSTRAINT
                    ? DatabaseResult::Conflict
                    : DatabaseResult::Error;

            cerr << "Failed to insert updated combination step: "
                 << sqlite3_errmsg(db) << endl;

            sqlite3_finalize(insertStepStatement);
            rollbackTransaction();

            return operationResult;
        }

        if (sqlite3_changes(db) == 0) {
            cerr << "Updated combination technique was not found or "
                 << "did not match the combination discipline."
                 << endl;

            sqlite3_finalize(insertStepStatement);
            rollbackTransaction();

            return DatabaseResult::NotFound;
        }
    }

    sqlite3_finalize(insertStepStatement);

    if (!commitTransaction()) {
        rollbackTransaction();
        return DatabaseResult::Error;
    }

    return DatabaseResult::Success;
}

// Deletes a combination belonging to a specific user
DatabaseResult Database::deleteCombatSportsCombination(
    int combinationId,
    int userId
) {
    lock_guard<recursive_mutex> lock(databaseMutex);

    const char* sql =
        "DELETE FROM combat_sports_combinations "
        "WHERE id = ? AND user_id = ?;";

    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare combination delete statement: "
             << sqlite3_errmsg(db) << endl;

        return DatabaseResult::Error;
    }

    result = sqlite3_bind_int(
        statement,
        1,
        combinationId
    );

    if (result == SQLITE_OK) {
        result = sqlite3_bind_int(
            statement,
            2,
            userId
        );
    }

    if (result != SQLITE_OK) {
        cerr << "Failed to bind combination delete identifiers: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    result = sqlite3_step(statement);

    if (result == SQLITE_DONE) {
        int deletedRows = sqlite3_changes(db);

        sqlite3_finalize(statement);

        if (deletedRows == 0) {
            return DatabaseResult::NotFound;
        }

        // Combination steps are removed automatically through ON DELETE
        // CASCADE when the parent combination is deleted.
        return DatabaseResult::Success;
    }

    // A combination referenced by a drill is protected by ON DELETE RESTRICT.
    if (result == SQLITE_CONSTRAINT) {
        cerr << "Combat sports combination deletion conflict: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Conflict;
    }

    cerr << "Failed to delete combat sports combination: "
         << sqlite3_errmsg(db) << endl;

    sqlite3_finalize(statement);

    return DatabaseResult::Error;
}

// Creates a drill and its ordered items within one transaction
DatabaseResult Database::createCombatSportsDrill(
    int userId,
    const string& discipline,
    const string& name,
    const string& instructions,
    const optional<int>& defaultDurationSeconds,
    const optional<int>& defaultRepetitions,
    const optional<int>& defaultRounds,
    const string& notes,
    const vector<CombatSportsDrillItemInput>& items,
    int& drillId
) {
    lock_guard<recursive_mutex> lock(databaseMutex);

    if (items.empty()) {
        cerr << "Cannot create a drill with no items." << endl;
        return DatabaseResult::Conflict;
    }

    if (!beginTransaction()) {
        return DatabaseResult::Error;
    }

    const char* drillSql =
        "INSERT INTO combat_sports_drills ("
        "user_id, discipline, name, instructions, "
        "default_duration_seconds, default_repetitions, "
        "default_rounds, notes"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* drillStatement = nullptr;

    int result = sqlite3_prepare_v2(
        db,
        drillSql,
        -1,
        &drillStatement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare combat sports drill insert: "
             << sqlite3_errmsg(db) << endl;

        rollbackTransaction();
        return DatabaseResult::Error;
    }

    result = sqlite3_bind_int(
        drillStatement,
        1,
        userId
    );

    if (result != SQLITE_OK ||
        !bindText(drillStatement, 2, discipline) ||
        !bindText(drillStatement, 3, name) ||
        !bindText(drillStatement, 4, instructions)) {

        sqlite3_finalize(drillStatement);
        rollbackTransaction();

        return DatabaseResult::Error;
    }

    // Optional numeric values are stored as SQL NULL when the user does
    // not provide them.
    result = defaultDurationSeconds.has_value()
        ? sqlite3_bind_int(
            drillStatement,
            5,
            defaultDurationSeconds.value()
        )
        : sqlite3_bind_null(drillStatement, 5);

    if (result == SQLITE_OK) {
        result = defaultRepetitions.has_value()
            ? sqlite3_bind_int(
                drillStatement,
                6,
                defaultRepetitions.value()
            )
            : sqlite3_bind_null(drillStatement, 6);
    }

    if (result == SQLITE_OK) {
        result = defaultRounds.has_value()
            ? sqlite3_bind_int(
                drillStatement,
                7,
                defaultRounds.value()
            )
            : sqlite3_bind_null(drillStatement, 7);
    }

    if (result != SQLITE_OK ||
        !bindText(drillStatement, 8, notes)) {

        cerr << "Failed to bind combat sports drill values: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(drillStatement);
        rollbackTransaction();

        return DatabaseResult::Error;
    }

    result = sqlite3_step(drillStatement);

    sqlite3_finalize(drillStatement);

    if (result != SQLITE_DONE) {
        DatabaseResult operationResult =
            result == SQLITE_CONSTRAINT
                ? DatabaseResult::Conflict
                : DatabaseResult::Error;

        cerr << "Failed to create combat sports drill: "
             << sqlite3_errmsg(db) << endl;

        rollbackTransaction();

        return operationResult;
    }

    int newDrillId = static_cast<int>(
        sqlite3_last_insert_rowid(db)
    );

    const char* techniqueItemSql =
        "INSERT INTO combat_sports_drill_items ("
        "user_id, drill_id, item_type, technique_id, "
        "combination_id, item_order"
        ") "
        "SELECT ?, ?, 'technique', id, NULL, ? "
        "FROM combat_sports_techniques "
        "WHERE id = ? "
        "AND user_id = ? "
        "AND discipline = ?;";

    const char* combinationItemSql =
        "INSERT INTO combat_sports_drill_items ("
        "user_id, drill_id, item_type, technique_id, "
        "combination_id, item_order"
        ") "
        "SELECT ?, ?, 'combination', NULL, id, ? "
        "FROM combat_sports_combinations "
        "WHERE id = ? "
        "AND user_id = ? "
        "AND discipline = ?;";

    for (size_t index = 0; index < items.size(); index++) {
        const CombatSportsDrillItemInput& item = items[index];

        const char* itemSql = nullptr;

        if (item.itemType == "technique") {
            itemSql = techniqueItemSql;
        }
        else if (item.itemType == "combination") {
            itemSql = combinationItemSql;
        }
        else {
            cerr << "Unsupported combat sports drill item type: "
                 << item.itemType << endl;

            rollbackTransaction();
            return DatabaseResult::Conflict;
        }

        sqlite3_stmt* itemStatement = nullptr;

        result = sqlite3_prepare_v2(
            db,
            itemSql,
            -1,
            &itemStatement,
            nullptr
        );

        if (result != SQLITE_OK) {
            cerr << "Failed to prepare combat sports drill item insert: "
                 << sqlite3_errmsg(db) << endl;

            rollbackTransaction();
            return DatabaseResult::Error;
        }

        int itemOrder = static_cast<int>(index) + 1;

        result = sqlite3_bind_int(
            itemStatement,
            1,
            userId
        );

        if (result == SQLITE_OK) {
            result = sqlite3_bind_int(
                itemStatement,
                2,
                newDrillId
            );
        }

        if (result == SQLITE_OK) {
            result = sqlite3_bind_int(
                itemStatement,
                3,
                itemOrder
            );
        }

        if (result == SQLITE_OK) {
            result = sqlite3_bind_int(
                itemStatement,
                4,
                item.referenceId
            );
        }

        if (result == SQLITE_OK) {
            result = sqlite3_bind_int(
                itemStatement,
                5,
                userId
            );
        }

        if (result != SQLITE_OK ||
            !bindText(itemStatement, 6, discipline)) {

            cerr << "Failed to bind combat sports drill item: "
                 << sqlite3_errmsg(db) << endl;

            sqlite3_finalize(itemStatement);
            rollbackTransaction();

            return DatabaseResult::Error;
        }

        result = sqlite3_step(itemStatement);

        if (result != SQLITE_DONE) {
            DatabaseResult operationResult =
                result == SQLITE_CONSTRAINT
                    ? DatabaseResult::Conflict
                    : DatabaseResult::Error;

            cerr << "Failed to insert combat sports drill item: "
                 << sqlite3_errmsg(db) << endl;

            sqlite3_finalize(itemStatement);
            rollbackTransaction();

            return operationResult;
        }

        if (sqlite3_changes(db) == 0) {
            cerr << "Drill item was not found or did not match "
                 << "the drill discipline."
                 << endl;

            sqlite3_finalize(itemStatement);
            rollbackTransaction();

            return DatabaseResult::NotFound;
        }

        sqlite3_finalize(itemStatement);
    }

    if (!commitTransaction()) {
        rollbackTransaction();
        return DatabaseResult::Error;
    }

    drillId = newDrillId;

    return DatabaseResult::Success;
}

// Retrieves one drill and its ordered technique or combination items
DatabaseResult Database::getCombatSportsDrill(
    int drillId,
    int userId,
    CombatSportsDrill& drill
) {
    lock_guard<recursive_mutex> lock(databaseMutex);

    const char* drillSql =
        "SELECT "
        "id, user_id, discipline, name, instructions, "
        "default_duration_seconds, default_repetitions, "
        "default_rounds, notes, created_at, updated_at "
        "FROM combat_sports_drills "
        "WHERE id = ? AND user_id = ?;";

    sqlite3_stmt* drillStatement = nullptr;

    int result = sqlite3_prepare_v2(
        db,
        drillSql,
        -1,
        &drillStatement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare combat sports drill lookup: "
             << sqlite3_errmsg(db) << endl;

        return DatabaseResult::Error;
    }

    result = sqlite3_bind_int(
        drillStatement,
        1,
        drillId
    );

    if (result == SQLITE_OK) {
        result = sqlite3_bind_int(
            drillStatement,
            2,
            userId
        );
    }

    if (result != SQLITE_OK) {
        cerr << "Failed to bind combat sports drill lookup values: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(drillStatement);
        return DatabaseResult::Error;
    }

    result = sqlite3_step(drillStatement);

    if (result == SQLITE_DONE) {
        sqlite3_finalize(drillStatement);
        return DatabaseResult::NotFound;
    }

    if (result != SQLITE_ROW) {
        cerr << "Failed to retrieve combat sports drill: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(drillStatement);
        return DatabaseResult::Error;
    }

    drill.id = sqlite3_column_int(drillStatement, 0);
    drill.userId = sqlite3_column_int(drillStatement, 1);

    drill.discipline = reinterpret_cast<const char*>(
        sqlite3_column_text(drillStatement, 2)
    );

    drill.name = reinterpret_cast<const char*>(
        sqlite3_column_text(drillStatement, 3)
    );

    const unsigned char* storedInstructions =
        sqlite3_column_text(drillStatement, 4);

    drill.instructions = storedInstructions != nullptr
        ? reinterpret_cast<const char*>(storedInstructions)
        : "";

    // Preserve the difference between an omitted setting and a numeric value.
    if (sqlite3_column_type(drillStatement, 5) == SQLITE_NULL) {
        drill.defaultDurationSeconds.reset();
    }
    else {
        drill.defaultDurationSeconds =
            sqlite3_column_int(drillStatement, 5);
    }

    if (sqlite3_column_type(drillStatement, 6) == SQLITE_NULL) {
        drill.defaultRepetitions.reset();
    }
    else {
        drill.defaultRepetitions =
            sqlite3_column_int(drillStatement, 6);
    }

    if (sqlite3_column_type(drillStatement, 7) == SQLITE_NULL) {
        drill.defaultRounds.reset();
    }
    else {
        drill.defaultRounds =
            sqlite3_column_int(drillStatement, 7);
    }

    const unsigned char* storedNotes =
        sqlite3_column_text(drillStatement, 8);

    drill.notes = storedNotes != nullptr
        ? reinterpret_cast<const char*>(storedNotes)
        : "";

    drill.createdAt = reinterpret_cast<const char*>(
        sqlite3_column_text(drillStatement, 9)
    );

    drill.updatedAt = reinterpret_cast<const char*>(
        sqlite3_column_text(drillStatement, 10)
    );

    drill.items.clear();

    sqlite3_finalize(drillStatement);

    const char* itemsSql =
        "SELECT "
        "items.id, items.user_id, items.drill_id, "
        "items.item_type, items.technique_id, "
        "items.combination_id, items.item_order, "
        "CASE "
            "WHEN items.item_type = 'technique' THEN techniques.name "
            "WHEN items.item_type = 'combination' THEN combinations.name "
            "ELSE NULL "
        "END AS item_name "
        "FROM combat_sports_drill_items AS items "
        "LEFT JOIN combat_sports_techniques AS techniques "
        "ON techniques.id = items.technique_id "
        "AND techniques.user_id = items.user_id "
        "LEFT JOIN combat_sports_combinations AS combinations "
        "ON combinations.id = items.combination_id "
        "AND combinations.user_id = items.user_id "
        "WHERE items.drill_id = ? "
        "AND items.user_id = ? "
        "ORDER BY items.item_order ASC;";

    sqlite3_stmt* itemsStatement = nullptr;

    result = sqlite3_prepare_v2(
        db,
        itemsSql,
        -1,
        &itemsStatement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare combat sports drill items lookup: "
             << sqlite3_errmsg(db) << endl;

        return DatabaseResult::Error;
    }

    result = sqlite3_bind_int(
        itemsStatement,
        1,
        drillId
    );

    if (result == SQLITE_OK) {
        result = sqlite3_bind_int(
            itemsStatement,
            2,
            userId
        );
    }

    if (result != SQLITE_OK) {
        cerr << "Failed to bind combat sports drill item lookup values: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(itemsStatement);
        return DatabaseResult::Error;
    }

    while ((result = sqlite3_step(itemsStatement)) == SQLITE_ROW) {
        CombatSportsDrillItem item;

        item.id = sqlite3_column_int(itemsStatement, 0);
        item.userId = sqlite3_column_int(itemsStatement, 1);
        item.drillId = sqlite3_column_int(itemsStatement, 2);

        item.itemType = reinterpret_cast<const char*>(
            sqlite3_column_text(itemsStatement, 3)
        );

        if (sqlite3_column_type(itemsStatement, 4) == SQLITE_NULL) {
            item.techniqueId.reset();
        }
        else {
            item.techniqueId =
                sqlite3_column_int(itemsStatement, 4);
        }

        if (sqlite3_column_type(itemsStatement, 5) == SQLITE_NULL) {
            item.combinationId.reset();
        }
        else {
            item.combinationId =
                sqlite3_column_int(itemsStatement, 5);
        }

        item.itemOrder = sqlite3_column_int(itemsStatement, 6);

        const unsigned char* storedItemName =
            sqlite3_column_text(itemsStatement, 7);

        if (storedItemName == nullptr) {
            cerr << "Combat sports drill item had no referenced name."
                 << endl;

            sqlite3_finalize(itemsStatement);
            drill.items.clear();

            return DatabaseResult::Error;
        }

        item.itemName =
            reinterpret_cast<const char*>(storedItemName);

        drill.items.push_back(item);
    }

    if (result == SQLITE_DONE) {
        sqlite3_finalize(itemsStatement);
        return DatabaseResult::Success;
    }

    cerr << "Failed to retrieve combat sports drill items: "
         << sqlite3_errmsg(db) << endl;

    sqlite3_finalize(itemsStatement);
    drill.items.clear();

    return DatabaseResult::Error;
}

// Retrieves all drills belonging to a specific user
DatabaseResult Database::getCombatSportsDrills(
    int userId,
    vector<CombatSportsDrill>& drills
) {
    lock_guard<recursive_mutex> lock(databaseMutex);

    const char* sql =
        "SELECT id "
        "FROM combat_sports_drills "
        "WHERE user_id = ? "
        "ORDER BY discipline ASC, name ASC, id ASC;";

    sqlite3_stmt* statement = nullptr;
    vector<int> drillIds;

    drills.clear();

    int result = sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare combat sports drills lookup: "
             << sqlite3_errmsg(db) << endl;

        return DatabaseResult::Error;
    }

    result = sqlite3_bind_int(
        statement,
        1,
        userId
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to bind combat sports drills owner ID: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        drillIds.push_back(
            sqlite3_column_int(statement, 0)
        );
    }

    if (result != SQLITE_DONE) {
        cerr << "Failed to retrieve combat sports drill IDs: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    sqlite3_finalize(statement);

    for (int storedDrillId : drillIds) {
        CombatSportsDrill drill;

        DatabaseResult lookupResult = getCombatSportsDrill(
            storedDrillId,
            userId,
            drill
        );

        if (lookupResult != DatabaseResult::Success) {
            drills.clear();
            return DatabaseResult::Error;
        }

        drills.push_back(drill);
    }

    return DatabaseResult::Success;
}

// Updates a drill and completely replaces its ordered items
DatabaseResult Database::updateCombatSportsDrill(
    int drillId,
    int userId,
    const string& discipline,
    const string& name,
    const string& instructions,
    const optional<int>& defaultDurationSeconds,
    const optional<int>& defaultRepetitions,
    const optional<int>& defaultRounds,
    const string& notes,
    const vector<CombatSportsDrillItemInput>& items
) {
    lock_guard<recursive_mutex> lock(databaseMutex);

    if (items.empty()) {
        cerr << "Cannot update a drill with no items." << endl;
        return DatabaseResult::Conflict;
    }

    if (!beginTransaction()) {
        return DatabaseResult::Error;
    }

    const char* updateSql =
        "UPDATE combat_sports_drills "
        "SET discipline = ?, "
        "name = ?, "
        "instructions = ?, "
        "default_duration_seconds = ?, "
        "default_repetitions = ?, "
        "default_rounds = ?, "
        "notes = ?, "
        "updated_at = CURRENT_TIMESTAMP "
        "WHERE id = ? AND user_id = ?;";

    sqlite3_stmt* updateStatement = nullptr;

    int result = sqlite3_prepare_v2(
        db,
        updateSql,
        -1,
        &updateStatement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare combat sports drill update: "
             << sqlite3_errmsg(db) << endl;

        rollbackTransaction();
        return DatabaseResult::Error;
    }

    if (!bindText(updateStatement, 1, discipline) ||
        !bindText(updateStatement, 2, name) ||
        !bindText(updateStatement, 3, instructions)) {

        sqlite3_finalize(updateStatement);
        rollbackTransaction();

        return DatabaseResult::Error;
    }

    result = defaultDurationSeconds.has_value()
        ? sqlite3_bind_int(
            updateStatement,
            4,
            defaultDurationSeconds.value()
        )
        : sqlite3_bind_null(updateStatement, 4);

    if (result == SQLITE_OK) {
        result = defaultRepetitions.has_value()
            ? sqlite3_bind_int(
                updateStatement,
                5,
                defaultRepetitions.value()
            )
            : sqlite3_bind_null(updateStatement, 5);
    }

    if (result == SQLITE_OK) {
        result = defaultRounds.has_value()
            ? sqlite3_bind_int(
                updateStatement,
                6,
                defaultRounds.value()
            )
            : sqlite3_bind_null(updateStatement, 6);
    }

    if (result != SQLITE_OK ||
        !bindText(updateStatement, 7, notes)) {

        cerr << "Failed to bind updated drill values: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(updateStatement);
        rollbackTransaction();

        return DatabaseResult::Error;
    }

    result = sqlite3_bind_int(
        updateStatement,
        8,
        drillId
    );

    if (result == SQLITE_OK) {
        result = sqlite3_bind_int(
            updateStatement,
            9,
            userId
        );
    }

    if (result != SQLITE_OK) {
        cerr << "Failed to bind updated drill identifiers: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(updateStatement);
        rollbackTransaction();

        return DatabaseResult::Error;
    }

    result = sqlite3_step(updateStatement);

    if (result != SQLITE_DONE) {
        DatabaseResult operationResult =
            result == SQLITE_CONSTRAINT
                ? DatabaseResult::Conflict
                : DatabaseResult::Error;

        cerr << "Failed to update combat sports drill: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(updateStatement);
        rollbackTransaction();

        return operationResult;
    }

    int updatedRows = sqlite3_changes(db);

    sqlite3_finalize(updateStatement);

    if (updatedRows == 0) {
        rollbackTransaction();
        return DatabaseResult::NotFound;
    }

    // Removing the old items inside the transaction allows rollback to
    // restore them if any replacement item is invalid.
    const char* deleteItemsSql =
        "DELETE FROM combat_sports_drill_items "
        "WHERE drill_id = ? AND user_id = ?;";

    sqlite3_stmt* deleteItemsStatement = nullptr;

    result = sqlite3_prepare_v2(
        db,
        deleteItemsSql,
        -1,
        &deleteItemsStatement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare old drill item deletion: "
             << sqlite3_errmsg(db) << endl;

        rollbackTransaction();
        return DatabaseResult::Error;
    }

    result = sqlite3_bind_int(
        deleteItemsStatement,
        1,
        drillId
    );

    if (result == SQLITE_OK) {
        result = sqlite3_bind_int(
            deleteItemsStatement,
            2,
            userId
        );
    }

    if (result != SQLITE_OK) {
        cerr << "Failed to bind old drill item identifiers: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(deleteItemsStatement);
        rollbackTransaction();

        return DatabaseResult::Error;
    }

    result = sqlite3_step(deleteItemsStatement);

    sqlite3_finalize(deleteItemsStatement);

    if (result != SQLITE_DONE) {
        cerr << "Failed to delete old combat sports drill items: "
             << sqlite3_errmsg(db) << endl;

        rollbackTransaction();
        return DatabaseResult::Error;
    }

    const char* techniqueItemSql =
        "INSERT INTO combat_sports_drill_items ("
        "user_id, drill_id, item_type, technique_id, "
        "combination_id, item_order"
        ") "
        "SELECT ?, ?, 'technique', id, NULL, ? "
        "FROM combat_sports_techniques "
        "WHERE id = ? "
        "AND user_id = ? "
        "AND discipline = ?;";

    const char* combinationItemSql =
        "INSERT INTO combat_sports_drill_items ("
        "user_id, drill_id, item_type, technique_id, "
        "combination_id, item_order"
        ") "
        "SELECT ?, ?, 'combination', NULL, id, ? "
        "FROM combat_sports_combinations "
        "WHERE id = ? "
        "AND user_id = ? "
        "AND discipline = ?;";

    for (size_t index = 0; index < items.size(); index++) {
        const CombatSportsDrillItemInput& item = items[index];

        const char* itemSql = nullptr;

        if (item.itemType == "technique") {
            itemSql = techniqueItemSql;
        }
        else if (item.itemType == "combination") {
            itemSql = combinationItemSql;
        }
        else {
            cerr << "Unsupported updated drill item type: "
                 << item.itemType << endl;

            rollbackTransaction();
            return DatabaseResult::Conflict;
        }

        sqlite3_stmt* itemStatement = nullptr;

        result = sqlite3_prepare_v2(
            db,
            itemSql,
            -1,
            &itemStatement,
            nullptr
        );

        if (result != SQLITE_OK) {
            cerr << "Failed to prepare updated drill item insert: "
                 << sqlite3_errmsg(db) << endl;

            rollbackTransaction();
            return DatabaseResult::Error;
        }

        int itemOrder = static_cast<int>(index) + 1;

        result = sqlite3_bind_int(
            itemStatement,
            1,
            userId
        );

        if (result == SQLITE_OK) {
            result = sqlite3_bind_int(
                itemStatement,
                2,
                drillId
            );
        }

        if (result == SQLITE_OK) {
            result = sqlite3_bind_int(
                itemStatement,
                3,
                itemOrder
            );
        }

        if (result == SQLITE_OK) {
            result = sqlite3_bind_int(
                itemStatement,
                4,
                item.referenceId
            );
        }

        if (result == SQLITE_OK) {
            result = sqlite3_bind_int(
                itemStatement,
                5,
                userId
            );
        }

        if (result != SQLITE_OK ||
            !bindText(itemStatement, 6, discipline)) {

            cerr << "Failed to bind updated drill item values: "
                 << sqlite3_errmsg(db) << endl;

            sqlite3_finalize(itemStatement);
            rollbackTransaction();

            return DatabaseResult::Error;
        }

        result = sqlite3_step(itemStatement);

        if (result != SQLITE_DONE) {
            DatabaseResult operationResult =
                result == SQLITE_CONSTRAINT
                    ? DatabaseResult::Conflict
                    : DatabaseResult::Error;

            cerr << "Failed to insert updated drill item: "
                 << sqlite3_errmsg(db) << endl;

            sqlite3_finalize(itemStatement);
            rollbackTransaction();

            return operationResult;
        }

        if (sqlite3_changes(db) == 0) {
            cerr << "Updated drill item was not found or did not "
                 << "match the drill discipline."
                 << endl;

            sqlite3_finalize(itemStatement);
            rollbackTransaction();

            return DatabaseResult::NotFound;
        }

        sqlite3_finalize(itemStatement);
    }

    if (!commitTransaction()) {
        rollbackTransaction();
        return DatabaseResult::Error;
    }

    return DatabaseResult::Success;
}

// Deletes a drill belonging to a specific user
DatabaseResult Database::deleteCombatSportsDrill(
    int drillId,
    int userId
) {
    lock_guard<recursive_mutex> lock(databaseMutex);

    const char* sql =
        "DELETE FROM combat_sports_drills "
        "WHERE id = ? AND user_id = ?;";

    sqlite3_stmt* statement = nullptr;

    int result = sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &statement,
        nullptr
    );

    if (result != SQLITE_OK) {
        cerr << "Failed to prepare combat sports drill deletion: "
             << sqlite3_errmsg(db) << endl;

        return DatabaseResult::Error;
    }

    result = sqlite3_bind_int(
        statement,
        1,
        drillId
    );

    if (result == SQLITE_OK) {
        result = sqlite3_bind_int(
            statement,
            2,
            userId
        );
    }

    if (result != SQLITE_OK) {
        cerr << "Failed to bind combat sports drill identifiers: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    result = sqlite3_step(statement);

    if (result == SQLITE_DONE) {
        int deletedRows = sqlite3_changes(db);

        sqlite3_finalize(statement);

        if (deletedRows == 0) {
            return DatabaseResult::NotFound;
        }

        // Ordered drill items are removed through ON DELETE CASCADE.
        return DatabaseResult::Success;
    }

    if (result == SQLITE_CONSTRAINT) {
        cerr << "Combat sports drill deletion conflict: "
             << sqlite3_errmsg(db) << endl;

        sqlite3_finalize(statement);
        return DatabaseResult::Conflict;
    }

    cerr << "Failed to delete combat sports drill: "
         << sqlite3_errmsg(db) << endl;

    sqlite3_finalize(statement);

    return DatabaseResult::Error;
}

namespace {

bool isBlankWorkoutText(const string& value) {
    return value.find_first_not_of(" \t\r\n") == string::npos;
}

bool bindWorkoutText(sqlite3_stmt* statement, int index, const string& value) {
    return sqlite3_bind_text(
        statement,
        index,
        value.c_str(),
        -1,
        SQLITE_TRANSIENT
    ) == SQLITE_OK;
}

bool containsWorkoutDiscipline(
    const vector<string>& disciplines,
    const string& discipline
) {
    for (const string& candidate : disciplines) {
        if (candidate == discipline) {
            return true;
        }
    }

    return false;
}

// Validates the shape before a transaction writes any nested records. The
// database constraints remain the final guard against invalid stored data.
bool validateWorkoutStructure(
    const string& name,
    const vector<string>& disciplines,
    const vector<CombatSportsWorkoutRoundInput>& rounds
) {
    if (isBlankWorkoutText(name) || disciplines.empty() || rounds.empty()) {
        return false;
    }

    for (const string& discipline : disciplines) {
        if (isBlankWorkoutText(discipline)) {
            return false;
        }
    }

    for (const CombatSportsWorkoutRoundInput& round : rounds) {
        if (round.activities.empty()) {
            return false;
        }

        for (const CombatSportsWorkoutActivityInput& activity :
             round.activities) {
            bool libraryActivity =
                activity.activityType == "technique" ||
                activity.activityType == "combination" ||
                activity.activityType == "drill";

            bool builtInActivity =
                activity.activityType == "jump_rope" ||
                activity.activityType == "conditioning" ||
                activity.activityType == "shadowboxing" ||
                activity.activityType == "rest" ||
                activity.activityType == "custom";

            bool validTarget =
                activity.targetType == "repetitions" ||
                activity.targetType == "duration_seconds" ||
                activity.targetType == "rounds";

            int referenceCount =
                (activity.techniqueId.has_value() ? 1 : 0) +
                (activity.combinationId.has_value() ? 1 : 0) +
                (activity.drillId.has_value() ? 1 : 0);

            bool correctReference =
                (activity.activityType == "technique" &&
                    activity.techniqueId.has_value()) ||
                (activity.activityType == "combination" &&
                    activity.combinationId.has_value()) ||
                (activity.activityType == "drill" &&
                    activity.drillId.has_value());

            if ((!libraryActivity && !builtInActivity) ||
                !validTarget ||
                activity.targetValue <= 0 ||
                activity.targetSets <= 0 ||
                activity.restAfterSeconds < 0 ||
                isBlankWorkoutText(activity.nameSnapshot) ||
                (libraryActivity &&
                    (referenceCount != 1 || !correctReference)) ||
                (builtInActivity && referenceCount != 0)) {
                return false;
            }
        }
    }

    return true;
}

DatabaseResult validateWorkoutLibraryReference(
    sqlite3* db,
    int userId,
    const vector<string>& disciplines,
    const CombatSportsWorkoutActivityInput& activity
) {
    const char* sql = nullptr;
    int referenceId = 0;

    if (activity.activityType == "technique") {
        sql = "SELECT discipline FROM combat_sports_techniques "
              "WHERE id = ? AND user_id = ?;";
        referenceId = activity.techniqueId.value();
    }
    else if (activity.activityType == "combination") {
        sql = "SELECT discipline FROM combat_sports_combinations "
              "WHERE id = ? AND user_id = ?;";
        referenceId = activity.combinationId.value();
    }
    else if (activity.activityType == "drill") {
        sql = "SELECT discipline FROM combat_sports_drills "
              "WHERE id = ? AND user_id = ?;";
        referenceId = activity.drillId.value();
    }
    else {
        return DatabaseResult::Success;
    }

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK ||
        sqlite3_bind_int(statement, 1, referenceId) != SQLITE_OK ||
        sqlite3_bind_int(statement, 2, userId) != SQLITE_OK) {
        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    int result = sqlite3_step(statement);

    if (result != SQLITE_ROW) {
        sqlite3_finalize(statement);
        return result == SQLITE_DONE
            ? DatabaseResult::Conflict
            : DatabaseResult::Error;
    }

    string discipline = reinterpret_cast<const char*>(
        sqlite3_column_text(statement, 0)
    );

    sqlite3_finalize(statement);

    return containsWorkoutDiscipline(disciplines, discipline)
        ? DatabaseResult::Success
        : DatabaseResult::Conflict;
}

DatabaseResult insertWorkoutChildren(
    sqlite3* db,
    int workoutTemplateId,
    int userId,
    const vector<string>& disciplines,
    const vector<CombatSportsWorkoutRoundInput>& rounds
) {
    const char* disciplineSql =
        "INSERT INTO combat_sports_workout_disciplines ("
        "user_id, workout_template_id, discipline) VALUES (?, ?, ?);";

    sqlite3_stmt* disciplineStatement = nullptr;

    if (sqlite3_prepare_v2(
            db,
            disciplineSql,
            -1,
            &disciplineStatement,
            nullptr
        ) != SQLITE_OK) {
        return DatabaseResult::Error;
    }

    for (const string& discipline : disciplines) {
        sqlite3_reset(disciplineStatement);
        sqlite3_clear_bindings(disciplineStatement);

        int result = sqlite3_bind_int(disciplineStatement, 1, userId);

        if (result == SQLITE_OK) {
            result = sqlite3_bind_int(
                disciplineStatement,
                2,
                workoutTemplateId
            );
        }

        if (result != SQLITE_OK ||
            !bindWorkoutText(disciplineStatement, 3, discipline) ||
            sqlite3_step(disciplineStatement) != SQLITE_DONE) {
            int errorCode = sqlite3_errcode(db);
            sqlite3_finalize(disciplineStatement);
            return errorCode == SQLITE_CONSTRAINT
                ? DatabaseResult::Conflict
                : DatabaseResult::Error;
        }
    }

    sqlite3_finalize(disciplineStatement);

    const char* roundSql =
        "INSERT INTO combat_sports_workout_rounds ("
        "user_id, workout_template_id, round_order, name, description"
        ") VALUES (?, ?, ?, ?, ?);";

    const char* activitySql =
        "INSERT INTO combat_sports_workout_activities ("
        "user_id, workout_template_id, workout_round_id, activity_order, "
        "activity_type, technique_id, combination_id, drill_id, "
        "name_snapshot, instructions_snapshot, target_type, target_value, "
        "target_sets, rest_after_seconds"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    for (size_t roundIndex = 0; roundIndex < rounds.size(); roundIndex++) {
        sqlite3_stmt* roundStatement = nullptr;

        if (sqlite3_prepare_v2(
                db,
                roundSql,
                -1,
                &roundStatement,
                nullptr
            ) != SQLITE_OK) {
            return DatabaseResult::Error;
        }

        const CombatSportsWorkoutRoundInput& round = rounds[roundIndex];

        int result = sqlite3_bind_int(roundStatement, 1, userId);

        if (result == SQLITE_OK) {
            result = sqlite3_bind_int(
                roundStatement,
                2,
                workoutTemplateId
            );
        }

        if (result == SQLITE_OK) {
            result = sqlite3_bind_int(
                roundStatement,
                3,
                static_cast<int>(roundIndex) + 1
            );
        }

        if (result != SQLITE_OK ||
            !bindWorkoutText(roundStatement, 4, round.name) ||
            !bindWorkoutText(roundStatement, 5, round.description) ||
            sqlite3_step(roundStatement) != SQLITE_DONE) {
            int errorCode = sqlite3_errcode(db);
            sqlite3_finalize(roundStatement);
            return errorCode == SQLITE_CONSTRAINT
                ? DatabaseResult::Conflict
                : DatabaseResult::Error;
        }

        sqlite3_finalize(roundStatement);

        int workoutRoundId = static_cast<int>(sqlite3_last_insert_rowid(db));

        for (size_t activityIndex = 0;
             activityIndex < round.activities.size();
             activityIndex++) {
            const CombatSportsWorkoutActivityInput& activity =
                round.activities[activityIndex];

            DatabaseResult referenceResult = validateWorkoutLibraryReference(
                db,
                userId,
                disciplines,
                activity
            );

            if (referenceResult != DatabaseResult::Success) {
                return referenceResult;
            }

            sqlite3_stmt* activityStatement = nullptr;

            if (sqlite3_prepare_v2(
                    db,
                    activitySql,
                    -1,
                    &activityStatement,
                    nullptr
                ) != SQLITE_OK) {
                return DatabaseResult::Error;
            }

            result = sqlite3_bind_int(activityStatement, 1, userId);
            result = result == SQLITE_OK
                ? sqlite3_bind_int(
                    activityStatement,
                    2,
                    workoutTemplateId
                )
                : result;
            result = result == SQLITE_OK
                ? sqlite3_bind_int(activityStatement, 3, workoutRoundId)
                : result;
            result = result == SQLITE_OK
                ? sqlite3_bind_int(
                    activityStatement,
                    4,
                    static_cast<int>(activityIndex) + 1
                )
                : result;

            bool textBound =
                result == SQLITE_OK &&
                bindWorkoutText(
                    activityStatement,
                    5,
                    activity.activityType
                );

            auto bindOptionalId = [&](int index, const optional<int>& value) {
                return value.has_value()
                    ? sqlite3_bind_int(activityStatement, index, value.value())
                    : sqlite3_bind_null(activityStatement, index);
            };

            if (textBound) {
                result = bindOptionalId(6, activity.techniqueId);
            }
            if (result == SQLITE_OK) {
                result = bindOptionalId(7, activity.combinationId);
            }
            if (result == SQLITE_OK) {
                result = bindOptionalId(8, activity.drillId);
            }

            textBound = result == SQLITE_OK &&
                bindWorkoutText(
                    activityStatement,
                    9,
                    activity.nameSnapshot
                ) &&
                bindWorkoutText(
                    activityStatement,
                    10,
                    activity.instructionsSnapshot
                ) &&
                bindWorkoutText(
                    activityStatement,
                    11,
                    activity.targetType
                );

            if (textBound) {
                result = sqlite3_bind_int(
                    activityStatement,
                    12,
                    activity.targetValue
                );
            }
            if (result == SQLITE_OK) {
                result = sqlite3_bind_int(
                    activityStatement,
                    13,
                    activity.targetSets
                );
            }
            if (result == SQLITE_OK) {
                result = sqlite3_bind_int(
                    activityStatement,
                    14,
                    activity.restAfterSeconds
                );
            }

            if (result != SQLITE_OK ||
                !textBound ||
                sqlite3_step(activityStatement) != SQLITE_DONE) {
                int errorCode = sqlite3_errcode(db);
                sqlite3_finalize(activityStatement);
                return errorCode == SQLITE_CONSTRAINT
                    ? DatabaseResult::Conflict
                    : DatabaseResult::Error;
            }

            sqlite3_finalize(activityStatement);
        }
    }

    return DatabaseResult::Success;
}

optional<int> readOptionalWorkoutInteger(sqlite3_stmt* statement, int column) {
    if (sqlite3_column_type(statement, column) == SQLITE_NULL) {
        return nullopt;
    }

    return sqlite3_column_int(statement, column);
}

string readWorkoutText(sqlite3_stmt* statement, int column) {
    const unsigned char* value = sqlite3_column_text(statement, column);
    return value == nullptr ? "" : reinterpret_cast<const char*>(value);
}

} // namespace

DatabaseResult Database::createCombatSportsWorkoutTemplate(
    int userId,
    const string& name,
    const string& description,
    const vector<string>& disciplines,
    const vector<CombatSportsWorkoutRoundInput>& rounds,
    int& workoutTemplateId
) {
    lock_guard<recursive_mutex> lock(databaseMutex);

    if (!validateWorkoutStructure(name, disciplines, rounds)) {
        return DatabaseResult::Conflict;
    }

    if (!beginTransaction()) {
        return DatabaseResult::Error;
    }

    const char* sql =
        "INSERT INTO combat_sports_workout_templates ("
        "user_id, name, description) VALUES (?, ?, ?);";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK ||
        sqlite3_bind_int(statement, 1, userId) != SQLITE_OK ||
        !bindText(statement, 2, name) ||
        !bindText(statement, 3, description)) {
        sqlite3_finalize(statement);
        rollbackTransaction();
        return DatabaseResult::Error;
    }

    int result = sqlite3_step(statement);
    sqlite3_finalize(statement);

    if (result != SQLITE_DONE) {
        DatabaseResult operationResult = result == SQLITE_CONSTRAINT
            ? DatabaseResult::Conflict
            : DatabaseResult::Error;
        rollbackTransaction();
        return operationResult;
    }

    int newTemplateId = static_cast<int>(sqlite3_last_insert_rowid(db));

    DatabaseResult childrenResult = insertWorkoutChildren(
        db,
        newTemplateId,
        userId,
        disciplines,
        rounds
    );

    if (childrenResult != DatabaseResult::Success) {
        rollbackTransaction();
        return childrenResult;
    }

    if (!commitTransaction()) {
        rollbackTransaction();
        return DatabaseResult::Error;
    }

    workoutTemplateId = newTemplateId;
    return DatabaseResult::Success;
}

DatabaseResult Database::getCombatSportsWorkoutTemplate(
    int workoutTemplateId,
    int userId,
    CombatSportsWorkoutTemplate& workoutTemplate
) {
    lock_guard<recursive_mutex> lock(databaseMutex);

    const char* templateSql =
        "SELECT id, user_id, name, description, created_at, updated_at "
        "FROM combat_sports_workout_templates "
        "WHERE id = ? AND user_id = ?;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            db,
            templateSql,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK ||
        sqlite3_bind_int(statement, 1, workoutTemplateId) != SQLITE_OK ||
        sqlite3_bind_int(statement, 2, userId) != SQLITE_OK) {
        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    int result = sqlite3_step(statement);

    if (result != SQLITE_ROW) {
        sqlite3_finalize(statement);
        return result == SQLITE_DONE
            ? DatabaseResult::NotFound
            : DatabaseResult::Error;
    }

    workoutTemplate = {};
    workoutTemplate.id = sqlite3_column_int(statement, 0);
    workoutTemplate.userId = sqlite3_column_int(statement, 1);
    workoutTemplate.name = readWorkoutText(statement, 2);
    workoutTemplate.description = readWorkoutText(statement, 3);
    workoutTemplate.createdAt = readWorkoutText(statement, 4);
    workoutTemplate.updatedAt = readWorkoutText(statement, 5);
    sqlite3_finalize(statement);

    const char* disciplineSql =
        "SELECT discipline FROM combat_sports_workout_disciplines "
        "WHERE workout_template_id = ? AND user_id = ? "
        "ORDER BY id;";

    if (sqlite3_prepare_v2(
            db,
            disciplineSql,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK ||
        sqlite3_bind_int(statement, 1, workoutTemplateId) != SQLITE_OK ||
        sqlite3_bind_int(statement, 2, userId) != SQLITE_OK) {
        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        workoutTemplate.disciplines.push_back(readWorkoutText(statement, 0));
    }

    sqlite3_finalize(statement);

    if (result != SQLITE_DONE) {
        return DatabaseResult::Error;
    }

    const char* nestedSql =
        "SELECT r.id, r.user_id, r.workout_template_id, r.round_order, "
        "r.name, r.description, r.created_at, r.updated_at, "
        "a.id, a.activity_order, a.activity_type, a.technique_id, "
        "a.combination_id, a.drill_id, a.name_snapshot, "
        "a.instructions_snapshot, a.target_type, a.target_value, "
        "a.target_sets, a.rest_after_seconds, a.created_at, a.updated_at "
        "FROM combat_sports_workout_rounds r "
        "LEFT JOIN combat_sports_workout_activities a "
        "ON a.workout_round_id = r.id AND a.user_id = r.user_id "
        "WHERE r.workout_template_id = ? AND r.user_id = ? "
        "ORDER BY r.round_order, a.activity_order;";

    if (sqlite3_prepare_v2(db, nestedSql, -1, &statement, nullptr) !=
            SQLITE_OK ||
        sqlite3_bind_int(statement, 1, workoutTemplateId) != SQLITE_OK ||
        sqlite3_bind_int(statement, 2, userId) != SQLITE_OK) {
        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    int currentRoundId = -1;

    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        int rowRoundId = sqlite3_column_int(statement, 0);

        if (rowRoundId != currentRoundId) {
            CombatSportsWorkoutRound round{};
            round.id = rowRoundId;
            round.userId = sqlite3_column_int(statement, 1);
            round.workoutTemplateId = sqlite3_column_int(statement, 2);
            round.roundOrder = sqlite3_column_int(statement, 3);
            round.name = readWorkoutText(statement, 4);
            round.description = readWorkoutText(statement, 5);
            round.createdAt = readWorkoutText(statement, 6);
            round.updatedAt = readWorkoutText(statement, 7);
            workoutTemplate.rounds.push_back(round);
            currentRoundId = rowRoundId;
        }

        if (sqlite3_column_type(statement, 8) != SQLITE_NULL) {
            CombatSportsWorkoutActivity activity{};
            activity.id = sqlite3_column_int(statement, 8);
            activity.userId = userId;
            activity.workoutTemplateId = workoutTemplateId;
            activity.workoutRoundId = rowRoundId;
            activity.activityOrder = sqlite3_column_int(statement, 9);
            activity.activityType = readWorkoutText(statement, 10);
            activity.techniqueId = readOptionalWorkoutInteger(statement, 11);
            activity.combinationId = readOptionalWorkoutInteger(statement, 12);
            activity.drillId = readOptionalWorkoutInteger(statement, 13);
            activity.nameSnapshot = readWorkoutText(statement, 14);
            activity.instructionsSnapshot = readWorkoutText(statement, 15);
            activity.targetType = readWorkoutText(statement, 16);
            activity.targetValue = sqlite3_column_int(statement, 17);
            activity.targetSets = sqlite3_column_int(statement, 18);
            activity.restAfterSeconds = sqlite3_column_int(statement, 19);
            activity.createdAt = readWorkoutText(statement, 20);
            activity.updatedAt = readWorkoutText(statement, 21);
            workoutTemplate.rounds.back().activities.push_back(activity);
        }
    }

    sqlite3_finalize(statement);
    return result == SQLITE_DONE
        ? DatabaseResult::Success
        : DatabaseResult::Error;
}

DatabaseResult Database::getCombatSportsWorkoutTemplates(
    int userId,
    vector<CombatSportsWorkoutTemplate>& workoutTemplates
) {
    lock_guard<recursive_mutex> lock(databaseMutex);
    workoutTemplates.clear();

    const char* sql =
        "SELECT id FROM combat_sports_workout_templates "
        "WHERE user_id = ? ORDER BY updated_at DESC, id DESC;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK ||
        sqlite3_bind_int(statement, 1, userId) != SQLITE_OK) {
        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    vector<int> templateIds;
    int result = SQLITE_OK;

    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        templateIds.push_back(sqlite3_column_int(statement, 0));
    }

    sqlite3_finalize(statement);

    if (result != SQLITE_DONE) {
        return DatabaseResult::Error;
    }

    for (int templateId : templateIds) {
        CombatSportsWorkoutTemplate workoutTemplate{};
        DatabaseResult templateResult = getCombatSportsWorkoutTemplate(
            templateId,
            userId,
            workoutTemplate
        );

        if (templateResult != DatabaseResult::Success) {
            return templateResult;
        }

        workoutTemplates.push_back(workoutTemplate);
    }

    return DatabaseResult::Success;
}

DatabaseResult Database::updateCombatSportsWorkoutTemplate(
    int workoutTemplateId,
    int userId,
    const string& name,
    const string& description,
    const vector<string>& disciplines,
    const vector<CombatSportsWorkoutRoundInput>& rounds
) {
    lock_guard<recursive_mutex> lock(databaseMutex);

    if (!validateWorkoutStructure(name, disciplines, rounds)) {
        return DatabaseResult::Conflict;
    }

    if (!beginTransaction()) {
        return DatabaseResult::Error;
    }

    const char* updateSql =
        "UPDATE combat_sports_workout_templates "
        "SET name = ?, description = ?, updated_at = CURRENT_TIMESTAMP "
        "WHERE id = ? AND user_id = ?;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, updateSql, -1, &statement, nullptr) !=
            SQLITE_OK ||
        !bindText(statement, 1, name) ||
        !bindText(statement, 2, description) ||
        sqlite3_bind_int(statement, 3, workoutTemplateId) != SQLITE_OK ||
        sqlite3_bind_int(statement, 4, userId) != SQLITE_OK) {
        sqlite3_finalize(statement);
        rollbackTransaction();
        return DatabaseResult::Error;
    }

    int result = sqlite3_step(statement);
    int changedRows = sqlite3_changes(db);
    sqlite3_finalize(statement);

    if (result != SQLITE_DONE) {
        DatabaseResult operationResult = result == SQLITE_CONSTRAINT
            ? DatabaseResult::Conflict
            : DatabaseResult::Error;
        rollbackTransaction();
        return operationResult;
    }

    if (changedRows == 0) {
        rollbackTransaction();
        return DatabaseResult::NotFound;
    }

    // Rounds cascade to their activities. Replacing nested rows keeps the
    // submitted order authoritative and avoids partially stale structures.
    const char* deleteRoundsSql =
        "DELETE FROM combat_sports_workout_rounds "
        "WHERE workout_template_id = ? AND user_id = ?;";
    const char* deleteDisciplinesSql =
        "DELETE FROM combat_sports_workout_disciplines "
        "WHERE workout_template_id = ? AND user_id = ?;";

    for (const char* deleteSql : {deleteRoundsSql, deleteDisciplinesSql}) {
        if (sqlite3_prepare_v2(
                db,
                deleteSql,
                -1,
                &statement,
                nullptr
            ) != SQLITE_OK ||
            sqlite3_bind_int(statement, 1, workoutTemplateId) != SQLITE_OK ||
            sqlite3_bind_int(statement, 2, userId) != SQLITE_OK ||
            sqlite3_step(statement) != SQLITE_DONE) {
            sqlite3_finalize(statement);
            rollbackTransaction();
            return DatabaseResult::Error;
        }

        sqlite3_finalize(statement);
        statement = nullptr;
    }

    DatabaseResult childrenResult = insertWorkoutChildren(
        db,
        workoutTemplateId,
        userId,
        disciplines,
        rounds
    );

    if (childrenResult != DatabaseResult::Success) {
        rollbackTransaction();
        return childrenResult;
    }

    if (!commitTransaction()) {
        rollbackTransaction();
        return DatabaseResult::Error;
    }

    return DatabaseResult::Success;
}

DatabaseResult Database::duplicateCombatSportsWorkoutTemplate(
    int workoutTemplateId,
    int userId,
    const string& duplicatedName,
    int& duplicatedWorkoutTemplateId
) {
    lock_guard<recursive_mutex> lock(databaseMutex);

    CombatSportsWorkoutTemplate source{};
    DatabaseResult sourceResult = getCombatSportsWorkoutTemplate(
        workoutTemplateId,
        userId,
        source
    );

    if (sourceResult != DatabaseResult::Success) {
        return sourceResult;
    }

    vector<CombatSportsWorkoutRoundInput> roundInputs;

    for (const CombatSportsWorkoutRound& round : source.rounds) {
        CombatSportsWorkoutRoundInput roundInput{};
        roundInput.name = round.name;
        roundInput.description = round.description;

        for (const CombatSportsWorkoutActivity& activity : round.activities) {
            roundInput.activities.push_back({
                activity.activityType,
                activity.techniqueId,
                activity.combinationId,
                activity.drillId,
                activity.nameSnapshot,
                activity.instructionsSnapshot,
                activity.targetType,
                activity.targetValue,
                activity.targetSets,
                activity.restAfterSeconds
            });
        }

        roundInputs.push_back(roundInput);
    }

    // Creation performs the copy as one transaction and revalidates every
    // library reference in case source content changed since it was saved.
    return createCombatSportsWorkoutTemplate(
        userId,
        duplicatedName,
        source.description,
        source.disciplines,
        roundInputs,
        duplicatedWorkoutTemplateId
    );
}

DatabaseResult Database::deleteCombatSportsWorkoutTemplate(
    int workoutTemplateId,
    int userId
) {
    lock_guard<recursive_mutex> lock(databaseMutex);

    const char* sql =
        "DELETE FROM combat_sports_workout_templates "
        "WHERE id = ? AND user_id = ?;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK ||
        sqlite3_bind_int(statement, 1, workoutTemplateId) != SQLITE_OK ||
        sqlite3_bind_int(statement, 2, userId) != SQLITE_OK) {
        sqlite3_finalize(statement);
        return DatabaseResult::Error;
    }

    int result = sqlite3_step(statement);
    int changedRows = sqlite3_changes(db);
    sqlite3_finalize(statement);

    if (result == SQLITE_DONE) {
        return changedRows == 0
            ? DatabaseResult::NotFound
            : DatabaseResult::Success;
    }

    return result == SQLITE_CONSTRAINT
        ? DatabaseResult::Conflict
        : DatabaseResult::Error;
}
