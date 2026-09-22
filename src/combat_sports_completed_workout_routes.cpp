#include "combat_sports_completed_workout_routes.h"

#include "auth.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace std;

namespace {

crow::response errorResponse(int statusCode, const string& error) {
    crow::json::wvalue body;
    body["success"] = false;
    body["error"] = error;
    crow::response response(statusCode, body);
    response.add_header("Content-Type", "application/json");
    return response;
}

bool authenticate(
    const crow::request& request,
    Database& database,
    int& userId,
    crow::response& failure
) {
    string error;
    AuthenticationResult result = authenticateRequest(
        request,
        database,
        userId,
        error
    );
    if (result == AuthenticationResult::Unauthorized) {
        failure = errorResponse(401, error);
        return false;
    }
    if (result == AuthenticationResult::Error) {
        failure = errorResponse(500, error);
        return false;
    }
    return true;
}

void writeOptionalInteger(
    crow::json::wvalue& json,
    const string& field,
    const optional<int>& value
) {
    if (value.has_value()) {
        json[field] = value.value();
    }
    else {
        json[field] = nullptr;
    }
}

crow::json::wvalue completedWorkoutToJson(
    const CombatSportsCompletedWorkout& workout
) {
    crow::json::wvalue json;
    json["id"] = workout.id;
    writeOptionalInteger(json, "workout_template_id", workout.workoutTemplateId);
    json["combat_sports_session_id"] = workout.combatSportsSessionId;
    json["workout_name"] = workout.workoutNameSnapshot;
    json["workout_description"] = workout.workoutDescriptionSnapshot;
    json["recording_method"] = workout.recordingMethod;
    json["started_at"] = workout.startedAt;
    json["completed_at"] = workout.completedAt;
    writeOptionalInteger(
        json,
        "planned_duration_seconds",
        workout.plannedDurationSeconds
    );
    json["actual_duration_seconds"] = workout.actualDurationSeconds;
    json["stopped_early"] = workout.stoppedEarly;
    json["notes"] = workout.notes;
    json["created_at"] = workout.createdAt;
    json["updated_at"] = workout.updatedAt;

    crow::json::wvalue::list rounds;
    for (const CombatSportsCompletedRound& round : workout.rounds) {
        crow::json::wvalue roundJson;
        roundJson["id"] = round.id;
        writeOptionalInteger(
            roundJson,
            "source_workout_round_id",
            round.sourceWorkoutRoundId
        );
        roundJson["round_order"] = round.roundOrder;
        roundJson["name"] = round.nameSnapshot;
        roundJson["description"] = round.descriptionSnapshot;
        roundJson["status"] = round.status;
        roundJson["created_at"] = round.createdAt;

        crow::json::wvalue::list activities;
        for (const CombatSportsCompletedActivity& activity : round.activities) {
            crow::json::wvalue activityJson;
            activityJson["id"] = activity.id;
            writeOptionalInteger(
                activityJson,
                "source_workout_activity_id",
                activity.sourceWorkoutActivityId
            );
            activityJson["activity_order"] = activity.activityOrder;
            activityJson["activity_type"] = activity.activityType;
            writeOptionalInteger(activityJson, "technique_id", activity.techniqueId);
            writeOptionalInteger(
                activityJson,
                "combination_id",
                activity.combinationId
            );
            writeOptionalInteger(activityJson, "drill_id", activity.drillId);
            activityJson["name"] = activity.nameSnapshot;
            activityJson["instructions"] = activity.instructionsSnapshot;
            activityJson["target_type"] = activity.targetType;
            writeOptionalInteger(activityJson, "planned_value", activity.plannedValue);
            writeOptionalInteger(activityJson, "planned_sets", activity.plannedSets);
            activityJson["completed_value"] = activity.completedValue;
            activityJson["completed_sets"] = activity.completedSets;
            writeOptionalInteger(
                activityJson,
                "actual_duration_seconds",
                activity.actualDurationSeconds
            );
            activityJson["status"] = activity.status;
            activityJson["was_unplanned"] = activity.wasUnplanned;
            activityJson["notes"] = activity.notes;
            activityJson["created_at"] = activity.createdAt;
            activities.push_back(move(activityJson));
        }
        roundJson["activities"] = move(activities);
        rounds.push_back(move(roundJson));
    }
    json["rounds"] = move(rounds);

    crow::json::wvalue::list totals;
    for (const CombatSportsCompletedTechniqueTotal& total :
         workout.techniqueTotals) {
        crow::json::wvalue totalJson;
        totalJson["id"] = total.id;
        writeOptionalInteger(totalJson, "technique_id", total.techniqueId);
        totalJson["technique_name"] = total.techniqueNameSnapshot;
        totalJson["technique_category"] = total.techniqueCategorySnapshot;
        totalJson["total_repetitions"] = total.totalRepetitions;
        totalJson["created_at"] = total.createdAt;
        totals.push_back(move(totalJson));
    }
    json["technique_totals"] = move(totals);
    return json;
}

bool readString(
    const crow::json::rvalue& object,
    const char* field,
    string& value,
    string& error,
    bool required = true
) {
    if (!object.has(field)) {
        if (!required) {
            value.clear();
            return true;
        }
        error = string(field) + " is required.";
        return false;
    }
    if (object[field].t() != crow::json::type::String) {
        error = string(field) + " must be a string.";
        return false;
    }
    value = object[field].s();
    return true;
}

bool readInteger(
    const crow::json::rvalue& object,
    const char* field,
    int& value,
    string& error,
    bool allowZero = false
) {
    if (!object.has(field) ||
        object[field].t() != crow::json::type::Number) {
        error = string(field) + " must be an integer.";
        return false;
    }
    int64_t parsed = object[field].i();
    if (parsed > numeric_limits<int>::max() ||
        parsed < (allowZero ? 0 : 1)) {
        error = string(field) + (allowZero
            ? " must be zero or greater."
            : " must be greater than zero.");
        return false;
    }
    value = static_cast<int>(parsed);
    return true;
}

bool readOptionalInteger(
    const crow::json::rvalue& object,
    const char* field,
    optional<int>& value,
    string& error,
    bool allowZero = false
) {
    if (!object.has(field) || object[field].t() == crow::json::type::Null) {
        value.reset();
        return true;
    }
    int parsed = 0;
    if (!readInteger(object, field, parsed, error, allowZero)) {
        return false;
    }
    value = parsed;
    return true;
}

bool readBoolean(
    const crow::json::rvalue& object,
    const char* field,
    bool& value,
    string& error
) {
    if (!object.has(field) ||
        (object[field].t() != crow::json::type::True &&
         object[field].t() != crow::json::type::False)) {
        error = string(field) + " must be a boolean.";
        return false;
    }
    value = object[field].b();
    return true;
}

bool parseActivity(
    const crow::json::rvalue& json,
    CombatSportsCompletedActivityInput& activity,
    string& error
) {
    if (json.t() != crow::json::type::Object) {
        error = "Each completed activity must be an object.";
        return false;
    }
    if (!readOptionalInteger(json, "source_workout_activity_id",
            activity.sourceWorkoutActivityId, error) ||
        !readString(json, "activity_type", activity.activityType, error) ||
        !readOptionalInteger(json, "technique_id", activity.techniqueId, error) ||
        !readOptionalInteger(json, "combination_id", activity.combinationId, error) ||
        !readOptionalInteger(json, "drill_id", activity.drillId, error) ||
        !readString(json, "name", activity.nameSnapshot, error) ||
        !readString(json, "instructions", activity.instructionsSnapshot,
            error, false) ||
        !readString(json, "target_type", activity.targetType, error) ||
        !readOptionalInteger(json, "planned_value", activity.plannedValue, error) ||
        !readOptionalInteger(json, "planned_sets", activity.plannedSets, error) ||
        !readInteger(json, "completed_value", activity.completedValue,
            error, true) ||
        !readInteger(json, "completed_sets", activity.completedSets,
            error, true) ||
        !readOptionalInteger(json, "actual_duration_seconds",
            activity.actualDurationSeconds, error) ||
        !readString(json, "status", activity.status, error) ||
        !readBoolean(json, "was_unplanned", activity.wasUnplanned, error) ||
        !readString(json, "notes", activity.notes, error, false)) {
        return false;
    }
    return true;
}

bool parseCompletedWorkout(
    const crow::json::rvalue& json,
    CombatSportsCompletedWorkoutInput& input,
    string& error
) {
    if (json.t() != crow::json::type::Object ||
        !readOptionalInteger(json, "workout_template_id",
            input.workoutTemplateId, error) ||
        !readInteger(json, "combat_sports_session_id",
            input.combatSportsSessionId, error) ||
        !readString(json, "workout_name", input.workoutNameSnapshot, error) ||
        !readString(json, "workout_description",
            input.workoutDescriptionSnapshot, error, false) ||
        !readString(json, "recording_method", input.recordingMethod, error) ||
        !readString(json, "started_at", input.startedAt, error) ||
        !readString(json, "completed_at", input.completedAt, error) ||
        !readInteger(json, "actual_duration_seconds",
            input.actualDurationSeconds, error) ||
        !readBoolean(json, "stopped_early", input.stoppedEarly, error) ||
        !readString(json, "notes", input.notes, error, false)) {
        return false;
    }
    if (!json.has("rounds") ||
        json["rounds"].t() != crow::json::type::List ||
        json["rounds"].size() == 0) {
        error = "At least one completed round is required.";
        return false;
    }

    for (const crow::json::rvalue& roundJson : json["rounds"]) {
        if (roundJson.t() != crow::json::type::Object) {
            error = "Each completed round must be an object.";
            return false;
        }
        CombatSportsCompletedRoundInput round;
        if (!readOptionalInteger(roundJson, "source_workout_round_id",
                round.sourceWorkoutRoundId, error) ||
            !readString(roundJson, "name", round.nameSnapshot, error) ||
            !readString(roundJson, "description",
                round.descriptionSnapshot, error, false)) {
            return false;
        }
        if (!roundJson.has("activities") ||
            roundJson["activities"].t() != crow::json::type::List ||
            roundJson["activities"].size() == 0) {
            error = "Each completed round requires at least one activity.";
            return false;
        }
        for (const crow::json::rvalue& activityJson :
             roundJson["activities"]) {
            CombatSportsCompletedActivityInput activity;
            if (!parseActivity(activityJson, activity, error)) {
                return false;
            }
            round.activities.push_back(move(activity));
        }
        input.rounds.push_back(move(round));
    }
    return true;
}

crow::response databaseFailure(DatabaseResult result) {
    if (result == DatabaseResult::NotFound) {
        return errorResponse(
            404,
            "A referenced session, workout, or completed workout was not found."
        );
    }
    if (result == DatabaseResult::Conflict) {
        return errorResponse(
            409,
            "The completed workout conflicts with existing data or contains an invalid result."
        );
    }
    return errorResponse(500, "Unable to process the completed workout.");
}

crow::response singleSuccess(
    int statusCode,
    const string& message,
    const CombatSportsCompletedWorkout& workout
) {
    crow::json::wvalue body;
    body["success"] = true;
    body["message"] = message;
    body["completed_workout"] = completedWorkoutToJson(workout);
    crow::response response(statusCode, body);
    response.add_header("Content-Type", "application/json");
    return response;
}

} // namespace

void registerCombatSportsCompletedWorkoutRoutes(
    crow::SimpleApp& app,
    Database& database
) {
    CROW_ROUTE(app, "/api/combat-sports/completed-workouts")
    .methods(crow::HTTPMethod::POST)
    ([&database](const crow::request& request) {
        int userId = 0;
        crow::response failure;
        if (!authenticate(request, database, userId, failure)) {
            return failure;
        }
        auto body = crow::json::load(request.body);
        if (!body) {
            return errorResponse(400, "Invalid completed workout data.");
        }
        CombatSportsCompletedWorkoutInput input{};
        string error;
        if (!parseCompletedWorkout(body, input, error)) {
            return errorResponse(400, error);
        }
        int completedWorkoutId = 0;
        DatabaseResult result = database.createCombatSportsCompletedWorkout(
            userId,
            input,
            completedWorkoutId
        );
        if (result != DatabaseResult::Success) {
            return databaseFailure(result);
        }
        CombatSportsCompletedWorkout workout{};
        result = database.getCombatSportsCompletedWorkout(
            completedWorkoutId,
            userId,
            workout
        );
        if (result != DatabaseResult::Success) {
            return errorResponse(
                500,
                "Completed workout was saved but could not be retrieved."
            );
        }
        return singleSuccess(
            201,
            "Completed workout saved successfully.",
            workout
        );
    });

    CROW_ROUTE(app, "/api/combat-sports/completed-workouts")
    .methods(crow::HTTPMethod::GET)
    ([&database](const crow::request& request) {
        int userId = 0;
        crow::response failure;
        if (!authenticate(request, database, userId, failure)) {
            return failure;
        }
        vector<CombatSportsCompletedWorkout> workouts;
        DatabaseResult result = database.getCombatSportsCompletedWorkouts(
            userId,
            workouts
        );
        if (result != DatabaseResult::Success) {
            return databaseFailure(result);
        }
        crow::json::wvalue::list list;
        for (const CombatSportsCompletedWorkout& workout : workouts) {
            list.push_back(completedWorkoutToJson(workout));
        }
        crow::json::wvalue body;
        body["success"] = true;
        body["completed_workouts"] = move(list);
        body["count"] = static_cast<int>(workouts.size());
        crow::response response(200, body);
        response.add_header("Content-Type", "application/json");
        return response;
    });

    CROW_ROUTE(app, "/api/combat-sports/completed-workouts/<int>")
    .methods(crow::HTTPMethod::GET)
    ([&database](const crow::request& request, int completedWorkoutId) {
        int userId = 0;
        crow::response failure;
        if (!authenticate(request, database, userId, failure)) {
            return failure;
        }
        if (completedWorkoutId <= 0) {
            return errorResponse(400, "Completed workout ID must be greater than zero.");
        }
        CombatSportsCompletedWorkout workout{};
        DatabaseResult result = database.getCombatSportsCompletedWorkout(
            completedWorkoutId,
            userId,
            workout
        );
        if (result != DatabaseResult::Success) {
            return databaseFailure(result);
        }
        return singleSuccess(
            200,
            "Completed workout retrieved successfully.",
            workout
        );
    });

    CROW_ROUTE(app, "/api/combat-sports/completed-workouts/session/<int>")
    .methods(crow::HTTPMethod::GET)
    ([&database](const crow::request& request, int sessionId) {
        int userId = 0;
        crow::response failure;
        if (!authenticate(request, database, userId, failure)) {
            return failure;
        }
        if (sessionId <= 0) {
            return errorResponse(400, "Session ID must be greater than zero.");
        }
        CombatSportsCompletedWorkout workout{};
        DatabaseResult result =
            database.getCombatSportsCompletedWorkoutBySession(
                sessionId,
                userId,
                workout
            );
        if (result != DatabaseResult::Success) {
            return databaseFailure(result);
        }
        return singleSuccess(
            200,
            "Completed workout retrieved successfully.",
            workout
        );
    });
}
