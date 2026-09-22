#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <mutex>
#include <optional>
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

// Represents one reusable combat-sports technique
struct CombatSportsTechnique {
    int id;
    int userId;
    std::string discipline;
    std::string name;
    std::string category;
    std::string description;
    std::string createdAt;
    std::string updatedAt;
};

// Represents one ordered technique within a combination
struct CombatSportsCombinationStep {
    int id;
    int userId;
    int combinationId;
    int techniqueId;
    int stepOrder;

    // Include the technique details needed when returning a complete
    // combination without requiring the caller to perform another lookup.
    std::string techniqueName;
    std::string techniqueCategory;
};

// Represents a combination and its ordered technique steps
struct CombatSportsCombination {
    int id;
    int userId;
    std::string discipline;
    std::string name;
    std::string description;
    std::string createdAt;
    std::string updatedAt;
    std::vector<CombatSportsCombinationStep> steps;
};

// Represents a technique or combination selected when creating or
// updating a drill
struct CombatSportsDrillItemInput {
    std::string itemType;
    int referenceId;
};

// Represents one ordered technique or combination within a drill
struct CombatSportsDrillItem {
    int id;
    int userId;
    int drillId;
    std::string itemType;

    // Only one reference will contain a value. This matches the schema's
    // rule that a drill item references either a technique or combination.
    std::optional<int> techniqueId;
    std::optional<int> combinationId;

    int itemOrder;

    // Store the referenced record's name for convenient API responses.
    std::string itemName;
};

// Represents a reusable drill and its ordered items
struct CombatSportsDrill {
    int id;
    int userId;
    std::string discipline;
    std::string name;
    std::string instructions;

    // Optional values remain distinct from zero. The database schema
    // permits NULL but rejects zero and negative values.
    std::optional<int> defaultDurationSeconds;
    std::optional<int> defaultRepetitions;
    std::optional<int> defaultRounds;

    std::string notes;
    std::string createdAt;
    std::string updatedAt;
    std::vector<CombatSportsDrillItem> items;
};

// Represents one activity supplied when creating or updating a workout.
// Library references are optional because built-in and custom activities do
// not point to a technique, combination, or drill.
struct CombatSportsWorkoutActivityInput {
    std::string activityType;
    std::optional<int> techniqueId;
    std::optional<int> combinationId;
    std::optional<int> drillId;
    std::string nameSnapshot;
    std::string instructionsSnapshot;
    std::string targetType;
    int targetValue;
    int targetSets;
    int restAfterSeconds;
};

// Represents one ordered round supplied with a complete workout template.
struct CombatSportsWorkoutRoundInput {
    std::string name;
    std::string description;
    std::vector<CombatSportsWorkoutActivityInput> activities;
};

// Represents one stored activity in a reusable workout template.
struct CombatSportsWorkoutActivity {
    int id;
    int userId;
    int workoutTemplateId;
    int workoutRoundId;
    int activityOrder;
    std::string activityType;
    std::optional<int> techniqueId;
    std::optional<int> combinationId;
    std::optional<int> drillId;
    std::string nameSnapshot;
    std::string instructionsSnapshot;
    std::string targetType;
    int targetValue;
    int targetSets;
    int restAfterSeconds;
    std::string createdAt;
    std::string updatedAt;
};

// Represents one stored round and its ordered activities.
struct CombatSportsWorkoutRound {
    int id;
    int userId;
    int workoutTemplateId;
    int roundOrder;
    std::string name;
    std::string description;
    std::string createdAt;
    std::string updatedAt;
    std::vector<CombatSportsWorkoutActivity> activities;
};

// Represents a complete reusable workout template.
struct CombatSportsWorkoutTemplate {
    int id;
    int userId;
    std::string name;
    std::string description;
    std::string createdAt;
    std::string updatedAt;
    std::vector<std::string> disciplines;
    std::vector<CombatSportsWorkoutRound> rounds;
};

// Handles the SQLite database connection for the app
class Database {
private:
    // Pointer to the active SQLite database connection
    sqlite3* db;

    // Location of the SQLite database file
    std::string databasePath;

    // Serializes access to the shared SQLite connection. A recursive mutex
    // allows one database method to safely call another locked method.
    std::recursive_mutex databaseMutex;

    // Safely binds a string value to a placeholder in a prepared SQL statement
    bool bindText(
        sqlite3_stmt* statement,
        int index,
        const std::string& value
    );

    // Starts a write transaction for a multi-statement database operation
    bool beginTransaction();

    // Permanently saves every statement in the active transaction
    bool commitTransaction();

    // Reverses every statement in the active transaction after a failure
    void rollbackTransaction();

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

    // Creates a reusable combat-sports technique for a specific user
    // Stores the generated technique ID in techniqueId
    DatabaseResult createCombatSportsTechnique(
        int userId,
        const std::string& discipline,
        const std::string& name,
        const std::string& category,
        const std::string& description,
        int& techniqueId
    );

    // Retrieves one technique belonging to a specific user
    DatabaseResult getCombatSportsTechnique(
        int techniqueId,
        int userId,
        CombatSportsTechnique& technique
    );

    // Retrieves all techniques belonging to a specific user
    DatabaseResult getCombatSportsTechniques(
        int userId,
        std::vector<CombatSportsTechnique>& techniques
    );

    // Updates a technique belonging to a specific user
    DatabaseResult updateCombatSportsTechnique(
        int techniqueId,
        int userId,
        const std::string& discipline,
        const std::string& name,
        const std::string& category,
        const std::string& description
    );

    // Deletes a technique belonging to a specific user
    DatabaseResult deleteCombatSportsTechnique(
        int techniqueId,
        int userId
    );

    // Creates a combination and its ordered technique steps as one
    // transaction. Repeated technique IDs are allowed.
    DatabaseResult createCombatSportsCombination(
        int userId,
        const std::string& discipline,
        const std::string& name,
        const std::string& description,
        const std::vector<int>& techniqueIds,
        int& combinationId
    );

    // Retrieves one combination and its ordered technique steps
    DatabaseResult getCombatSportsCombination(
        int combinationId,
        int userId,
        CombatSportsCombination& combination
    );

    // Retrieves all combinations belonging to a specific user
    DatabaseResult getCombatSportsCombinations(
        int userId,
        std::vector<CombatSportsCombination>& combinations
    );

    // Updates a combination and completely replaces its ordered steps
    // within one transaction
    DatabaseResult updateCombatSportsCombination(
        int combinationId,
        int userId,
        const std::string& discipline,
        const std::string& name,
        const std::string& description,
        const std::vector<int>& techniqueIds
    );

    // Deletes a combination belonging to a specific user
    DatabaseResult deleteCombatSportsCombination(
        int combinationId,
        int userId
    );

    // Creates a drill and its ordered technique or combination items
    // within one transaction
    DatabaseResult createCombatSportsDrill(
        int userId,
        const std::string& discipline,
        const std::string& name,
        const std::string& instructions,
        const std::optional<int>& defaultDurationSeconds,
        const std::optional<int>& defaultRepetitions,
        const std::optional<int>& defaultRounds,
        const std::string& notes,
        const std::vector<CombatSportsDrillItemInput>& items,
        int& drillId
    );

    // Retrieves one drill and its ordered technique or combination items
    DatabaseResult getCombatSportsDrill(
        int drillId,
        int userId,
        CombatSportsDrill& drill
    );

    // Retrieves all drills belonging to a specific user
    DatabaseResult getCombatSportsDrills(
        int userId,
        std::vector<CombatSportsDrill>& drills
    );

    // Updates a drill and completely replaces its ordered items within
    // one transaction
    DatabaseResult updateCombatSportsDrill(
        int drillId,
        int userId,
        const std::string& discipline,
        const std::string& name,
        const std::string& instructions,
        const std::optional<int>& defaultDurationSeconds,
        const std::optional<int>& defaultRepetitions,
        const std::optional<int>& defaultRounds,
        const std::string& notes,
        const std::vector<CombatSportsDrillItemInput>& items
    );

    // Deletes a drill belonging to a specific user
    DatabaseResult deleteCombatSportsDrill(
        int drillId,
        int userId
    );

    // Creates a complete workout template as one transaction.
    DatabaseResult createCombatSportsWorkoutTemplate(
        int userId,
        const std::string& name,
        const std::string& description,
        const std::vector<std::string>& disciplines,
        const std::vector<CombatSportsWorkoutRoundInput>& rounds,
        int& workoutTemplateId
    );

    // Retrieves one template with its disciplines, rounds, and activities.
    DatabaseResult getCombatSportsWorkoutTemplate(
        int workoutTemplateId,
        int userId,
        CombatSportsWorkoutTemplate& workoutTemplate
    );

    // Retrieves every workout template belonging to a user.
    DatabaseResult getCombatSportsWorkoutTemplates(
        int userId,
        std::vector<CombatSportsWorkoutTemplate>& workoutTemplates
    );

    // Replaces a template's complete nested structure transactionally.
    DatabaseResult updateCombatSportsWorkoutTemplate(
        int workoutTemplateId,
        int userId,
        const std::string& name,
        const std::string& description,
        const std::vector<std::string>& disciplines,
        const std::vector<CombatSportsWorkoutRoundInput>& rounds
    );

    // Copies a complete template under a new user-owned name.
    DatabaseResult duplicateCombatSportsWorkoutTemplate(
        int workoutTemplateId,
        int userId,
        const std::string& duplicatedName,
        int& duplicatedWorkoutTemplateId
    );

    // Deletes one user-owned template without deleting completed snapshots.
    DatabaseResult deleteCombatSportsWorkoutTemplate(
        int workoutTemplateId,
        int userId
    );
};

#endif
