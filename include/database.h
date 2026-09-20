#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>
#include <vector>

// Represents the possible outcomes of a database operation
enum class DatabaseResult {
    Success,
    NotFound,
    Conflict,
    Error
};

// Represents one combat-sports session retrieved from the database
struct CombatSportsSession {
    int id;
    int userId;
    std::string discipline;
    std::string trainingType;
    std::string sessionDate;
    int durationMinutes;
    std::string recordingMethod;
    std::string notes;
    std::string createdAt;
    std::string updatedAt;
};

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

    // Creates a new user account in the database
    // Returns the result of the user creation database operation
    DatabaseResult createUser(
        const std::string& username,
        const std::string& email,
        const std::string& passwordHash
    );

    // Retrieves a user's password hash using their username or email
    DatabaseResult getUserLoginData(
        const std::string& login,
        int& userId,
        std::string& passwordHash
    );

    // Creates a new authenticated session for a user
    DatabaseResult createSession(
        int userId,
        const std::string& sessionToken,
        const std::string& expiresAt
    );

    // Deletes an authenticated session using its session token
    DatabaseResult deleteSession(
        const std::string& sessionToken
    );

    // Checks whether a session token belongs to a valid, unexpired session
    DatabaseResult validateSession(
        const std::string& sessionToken,
        int& userId
    );

    // Creates a completed combat-sports training session for a user
    // Stores the generated session ID in sessionId
    DatabaseResult createCombatSportsSession(
        int userId,
        const std::string& discipline,
        const std::string& trainingType,
        const std::string& sessionDate,
        int durationMinutes,
        const std::string& recordingMethod,
        const std::string& notes,
        int& sessionId
    );

    // Retrieves all combat-sports sessions belonging to a user
    DatabaseResult getCombatSportsSessions(
        int userId,
        std::vector<CombatSportsSession>& sessions
    );

        // Updates a combat-sports session belonging to a specific user
    DatabaseResult updateCombatSportsSession(
        int sessionId,
        int userId,
        const std::string& discipline,
        const std::string& trainingType,
        const std::string& sessionDate,
        int durationMinutes,
        const std::string& recordingMethod,
        const std::string& notes
    );

        // Deletes a combat-sports session belonging to a specific user
    DatabaseResult deleteCombatSportsSession(
        int sessionId,
        int userId
    );
};

#endif
