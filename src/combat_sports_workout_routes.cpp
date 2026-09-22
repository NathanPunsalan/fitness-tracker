#include "combat_sports_workout_routes.h"

#include "auth.h"
#include "validation.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace std;

namespace {

crow::response createWorkoutErrorResponse(
    int statusCode,
    const string& error
) {
    crow::json::wvalue body;
    body["success"] = false;
    body["error"] = error;

    crow::response response(statusCode, body);
    response.add_header("Content-Type", "application/json");
    return response;
}

bool authenticateWorkoutRequest(
    const crow::request& request,
    Database& database,
    int& userId,
    crow::response& failureResponse
) {
    string authenticationError;
    AuthenticationResult result = authenticateRequest(
        request,
        database,
        userId,
        authenticationError
    );

    if (result == AuthenticationResult::Unauthorized) {
        failureResponse = createWorkoutErrorResponse(
            401,
            authenticationError
        );
        return false;
    }

    if (result == AuthenticationResult::Error) {
        failureResponse = createWorkoutErrorResponse(
            500,
            authenticationError
        );
        return false;
    }

    return true;
}

void writeOptionalId(
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

crow::json::wvalue workoutToJson(
    const CombatSportsWorkoutTemplate& workout
) {
    crow::json::wvalue json;
    json["id"] = workout.id;
    json["name"] = workout.name;
    json["description"] = workout.description;
    json["created_at"] = workout.createdAt;
    json["updated_at"] = workout.updatedAt;

    crow::json::wvalue::list disciplines;
    for (const string& discipline : workout.disciplines) {
        disciplines.push_back(discipline);
    }
    json["disciplines"] = move(disciplines);

    crow::json::wvalue::list rounds;

    for (const CombatSportsWorkoutRound& round : workout.rounds) {
        crow::json::wvalue roundJson;
        roundJson["id"] = round.id;
        roundJson["round_order"] = round.roundOrder;
        roundJson["name"] = round.name;
        roundJson["description"] = round.description;
        roundJson["created_at"] = round.createdAt;
        roundJson["updated_at"] = round.updatedAt;

        crow::json::wvalue::list activities;

        for (const CombatSportsWorkoutActivity& activity :
             round.activities) {
            crow::json::wvalue activityJson;
            activityJson["id"] = activity.id;
            activityJson["activity_order"] = activity.activityOrder;
            activityJson["activity_type"] = activity.activityType;
            writeOptionalId(
                activityJson,
                "technique_id",
                activity.techniqueId
            );
            writeOptionalId(
                activityJson,
                "combination_id",
                activity.combinationId
            );
            writeOptionalId(
                activityJson,
                "drill_id",
                activity.drillId
            );
            activityJson["name"] = activity.nameSnapshot;
            activityJson["instructions"] =
                activity.instructionsSnapshot;
            activityJson["target_type"] = activity.targetType;
            activityJson["target_value"] = activity.targetValue;
            activityJson["target_sets"] = activity.targetSets;
            activityJson["rest_after_seconds"] =
                activity.restAfterSeconds;
            activityJson["created_at"] = activity.createdAt;
            activityJson["updated_at"] = activity.updatedAt;
            activities.push_back(move(activityJson));
        }

        roundJson["activities"] = move(activities);
        rounds.push_back(move(roundJson));
    }

    json["rounds"] = move(rounds);
    return json;
}

bool parsePositiveInteger(
    const crow::json::rvalue& object,
    const string& field,
    int& output,
    string& error
) {
    if (!object.has(field.c_str()) ||
        object[field.c_str()].t() != crow::json::type::Number) {
        error = field + " must be a positive integer.";
        return false;
    }

    int64_t value = object[field.c_str()].i();
    if (value <= 0 || value > numeric_limits<int>::max()) {
        error = field + " must be a positive integer.";
        return false;
    }

    output = static_cast<int>(value);
    return true;
}

bool parseOptionalReferenceId(
    const crow::json::rvalue& activity,
    const string& field,
    optional<int>& output,
    string& error
) {
    if (!activity.has(field.c_str()) ||
        activity[field.c_str()].t() == crow::json::type::Null) {
        output.reset();
        return true;
    }

    if (activity[field.c_str()].t() != crow::json::type::Number) {
        error = field + " must be a positive integer or null.";
        return false;
    }

    int64_t value = activity[field.c_str()].i();
    if (value <= 0 || value > numeric_limits<int>::max()) {
        error = field + " must be a positive integer or null.";
        return false;
    }

    output = static_cast<int>(value);
    return true;
}

bool parseActivity(
    const crow::json::rvalue& json,
    CombatSportsWorkoutActivityInput& activity,
    string& error
) {
    if (json.t() != crow::json::type::Object) {
        error = "Every workout activity must be an object.";
        return false;
    }

    if (!json.has("activity_type") ||
        !json.has("name") ||
        !json.has("target_type") ||
        !json.has("target_value")) {
        error = "Every activity requires a type, name, target type, and target value.";
        return false;
    }

    if (json["activity_type"].t() != crow::json::type::String ||
        json["name"].t() != crow::json::type::String ||
        json["target_type"].t() != crow::json::type::String) {
        error = "Activity type, name, and target type must be text values.";
        return false;
    }

    if (json.has("instructions") &&
        json["instructions"].t() != crow::json::type::String) {
        error = "Activity instructions must be a text value.";
        return false;
    }

    activity.activityType = json["activity_type"].s();
    activity.nameSnapshot = json["name"].s();
    activity.instructionsSnapshot = json.has("instructions")
        ? string(json["instructions"].s())
        : "";
    activity.targetType = json["target_type"].s();

    if (isBlank(activity.nameSnapshot)) {
        error = "Activity name cannot be empty.";
        return false;
    }

    const vector<string> supportedActivityTypes = {
        "technique", "combination", "drill", "jump_rope",
        "conditioning", "shadowboxing", "rest", "custom"
    };
    bool supportedActivity = false;
    for (const string& type : supportedActivityTypes) {
        if (activity.activityType == type) {
            supportedActivity = true;
            break;
        }
    }

    if (!supportedActivity) {
        error = "Unsupported workout activity type.";
        return false;
    }

    if (activity.targetType != "repetitions" &&
        activity.targetType != "duration_seconds" &&
        activity.targetType != "rounds") {
        error = "Unsupported workout activity target type.";
        return false;
    }

    if (!parsePositiveInteger(
            json,
            "target_value",
            activity.targetValue,
            error)) {
        return false;
    }

    activity.targetSets = 1;
    if (json.has("target_sets")) {
        if (!parsePositiveInteger(
                json,
                "target_sets",
                activity.targetSets,
                error)) {
            return false;
        }
    }

    activity.restAfterSeconds = 0;
    if (json.has("rest_after_seconds")) {
        if (json["rest_after_seconds"].t() != crow::json::type::Number ||
            json["rest_after_seconds"].i() < 0 ||
            json["rest_after_seconds"].i() >
                numeric_limits<int>::max()) {
            error = "rest_after_seconds must be a non-negative integer.";
            return false;
        }

        activity.restAfterSeconds = static_cast<int>(
            json["rest_after_seconds"].i()
        );
    }

    if (!parseOptionalReferenceId(
            json,
            "technique_id",
            activity.techniqueId,
            error) ||
        !parseOptionalReferenceId(
            json,
            "combination_id",
            activity.combinationId,
            error) ||
        !parseOptionalReferenceId(
            json,
            "drill_id",
            activity.drillId,
            error)) {
        return false;
    }

    int referenceCount =
        (activity.techniqueId.has_value() ? 1 : 0) +
        (activity.combinationId.has_value() ? 1 : 0) +
        (activity.drillId.has_value() ? 1 : 0);

    bool validReference =
        (activity.activityType == "technique" &&
            activity.techniqueId.has_value()) ||
        (activity.activityType == "combination" &&
            activity.combinationId.has_value()) ||
        (activity.activityType == "drill" &&
            activity.drillId.has_value());

    bool libraryActivity =
        activity.activityType == "technique" ||
        activity.activityType == "combination" ||
        activity.activityType == "drill";

    if ((libraryActivity && (referenceCount != 1 || !validReference)) ||
        (!libraryActivity && referenceCount != 0)) {
        error = "Activity references do not match the selected activity type.";
        return false;
    }

    return true;
}

bool parseWorkoutFields(
    const crow::json::rvalue& body,
    string& name,
    string& description,
    vector<string>& disciplines,
    vector<CombatSportsWorkoutRoundInput>& rounds,
    string& error
) {
    if (body.t() != crow::json::type::Object ||
        !body.has("name") ||
        !body.has("disciplines") ||
        !body.has("rounds")) {
        error = "Workout name, disciplines, and rounds are required.";
        return false;
    }

    if (body["name"].t() != crow::json::type::String ||
        body["disciplines"].t() != crow::json::type::List ||
        body["rounds"].t() != crow::json::type::List) {
        error = "Workout name, disciplines, or rounds have invalid data types.";
        return false;
    }

    if (body.has("description") &&
        body["description"].t() != crow::json::type::String) {
        error = "Workout description must be a text value.";
        return false;
    }

    name = body["name"].s();
    description = body.has("description")
        ? string(body["description"].s())
        : "";

    if (isBlank(name)) {
        error = "Workout name cannot be empty.";
        return false;
    }

    disciplines.clear();
    for (const crow::json::rvalue& discipline : body["disciplines"]) {
        if (discipline.t() != crow::json::type::String ||
            isBlank(string(discipline.s()))) {
            error = "Every workout discipline must be non-empty text.";
            return false;
        }
        disciplines.push_back(discipline.s());
    }

    if (disciplines.empty()) {
        error = "A workout must contain at least one discipline.";
        return false;
    }

    rounds.clear();
    for (const crow::json::rvalue& roundJson : body["rounds"]) {
        if (roundJson.t() != crow::json::type::Object ||
            !roundJson.has("activities") ||
            roundJson["activities"].t() != crow::json::type::List) {
            error = "Every workout round requires an activity list.";
            return false;
        }

        if (roundJson.has("name") &&
            roundJson["name"].t() != crow::json::type::String) {
            error = "Workout round name must be a text value.";
            return false;
        }
        if (roundJson.has("description") &&
            roundJson["description"].t() != crow::json::type::String) {
            error = "Workout round description must be a text value.";
            return false;
        }

        CombatSportsWorkoutRoundInput round;
        round.name = roundJson.has("name")
            ? string(roundJson["name"].s())
            : "";
        round.description = roundJson.has("description")
            ? string(roundJson["description"].s())
            : "";

        for (const crow::json::rvalue& activityJson :
             roundJson["activities"]) {
            CombatSportsWorkoutActivityInput activity;
            if (!parseActivity(activityJson, activity, error)) {
                return false;
            }
            round.activities.push_back(activity);
        }

        if (round.activities.empty()) {
            error = "Every workout round must contain at least one activity.";
            return false;
        }

        rounds.push_back(round);
    }

    if (rounds.empty()) {
        error = "A workout must contain at least one round.";
        return false;
    }

    return true;
}

crow::response workoutDatabaseFailure(
    DatabaseResult result,
    const string& action
) {
    if (result == DatabaseResult::NotFound) {
        return createWorkoutErrorResponse(
            404,
            "Workout template not found."
        );
    }

    if (result == DatabaseResult::Conflict) {
        return createWorkoutErrorResponse(
            409,
            "Workout name already exists or the workout contains invalid or unavailable content."
        );
    }

    return createWorkoutErrorResponse(
        500,
        "Unable to " + action + " workout template."
    );
}

crow::response workoutSuccessResponse(
    int statusCode,
    const string& message,
    const CombatSportsWorkoutTemplate& workout
) {
    crow::json::wvalue body;
    body["success"] = true;
    body["message"] = message;
    body["workout"] = workoutToJson(workout);

    crow::response response(statusCode, body);
    response.add_header("Content-Type", "application/json");
    return response;
}

} // namespace

void registerCombatSportsWorkoutRoutes(
    crow::SimpleApp& app,
    Database& database
) {
    CROW_ROUTE(app, "/api/combat-sports/workouts")
    .methods(crow::HTTPMethod::POST)
    ([&database](const crow::request& request) {
        int userId;
        crow::response authenticationFailure;
        if (!authenticateWorkoutRequest(
                request,
                database,
                userId,
                authenticationFailure)) {
            return authenticationFailure;
        }

        auto body = crow::json::load(request.body);
        if (!body) {
            return createWorkoutErrorResponse(400, "Invalid workout data.");
        }

        string name, description, error;
        vector<string> disciplines;
        vector<CombatSportsWorkoutRoundInput> rounds;
        if (!parseWorkoutFields(
                body,
                name,
                description,
                disciplines,
                rounds,
                error)) {
            return createWorkoutErrorResponse(400, error);
        }

        int workoutId = 0;
        DatabaseResult result =
            database.createCombatSportsWorkoutTemplate(
                userId,
                name,
                description,
                disciplines,
                rounds,
                workoutId
            );

        if (result != DatabaseResult::Success) {
            return workoutDatabaseFailure(result, "create");
        }

        CombatSportsWorkoutTemplate workout;
        result = database.getCombatSportsWorkoutTemplate(
            workoutId,
            userId,
            workout
        );

        if (result != DatabaseResult::Success) {
            return createWorkoutErrorResponse(
                500,
                "Workout was created but could not be retrieved."
            );
        }

        return workoutSuccessResponse(
            201,
            "Workout template created successfully.",
            workout
        );
    });

    CROW_ROUTE(app, "/api/combat-sports/workouts")
    .methods(crow::HTTPMethod::GET)
    ([&database](const crow::request& request) {
        int userId;
        crow::response authenticationFailure;
        if (!authenticateWorkoutRequest(
                request,
                database,
                userId,
                authenticationFailure)) {
            return authenticationFailure;
        }

        vector<CombatSportsWorkoutTemplate> workouts;
        DatabaseResult result =
            database.getCombatSportsWorkoutTemplates(userId, workouts);

        if (result != DatabaseResult::Success) {
            return workoutDatabaseFailure(result, "retrieve");
        }

        crow::json::wvalue::list list;
        for (const CombatSportsWorkoutTemplate& workout : workouts) {
            list.push_back(workoutToJson(workout));
        }

        crow::json::wvalue body;
        body["success"] = true;
        body["workouts"] = move(list);
        body["count"] = static_cast<int>(workouts.size());

        crow::response response(200, body);
        response.add_header("Content-Type", "application/json");
        return response;
    });

    CROW_ROUTE(app, "/api/combat-sports/workouts/<int>")
    .methods(crow::HTTPMethod::GET)
    ([&database](const crow::request& request, int workoutId) {
        int userId;
        crow::response authenticationFailure;
        if (!authenticateWorkoutRequest(
                request,
                database,
                userId,
                authenticationFailure)) {
            return authenticationFailure;
        }
        if (workoutId <= 0) {
            return createWorkoutErrorResponse(
                400,
                "Workout ID must be greater than zero."
            );
        }

        CombatSportsWorkoutTemplate workout;
        DatabaseResult result = database.getCombatSportsWorkoutTemplate(
            workoutId,
            userId,
            workout
        );
        if (result != DatabaseResult::Success) {
            return workoutDatabaseFailure(result, "retrieve");
        }

        return workoutSuccessResponse(
            200,
            "Workout template retrieved successfully.",
            workout
        );
    });

    CROW_ROUTE(app, "/api/combat-sports/workouts/<int>")
    .methods(crow::HTTPMethod::PUT)
    ([&database](const crow::request& request, int workoutId) {
        int userId;
        crow::response authenticationFailure;
        if (!authenticateWorkoutRequest(
                request,
                database,
                userId,
                authenticationFailure)) {
            return authenticationFailure;
        }
        if (workoutId <= 0) {
            return createWorkoutErrorResponse(
                400,
                "Workout ID must be greater than zero."
            );
        }

        auto body = crow::json::load(request.body);
        if (!body) {
            return createWorkoutErrorResponse(400, "Invalid workout data.");
        }

        string name, description, error;
        vector<string> disciplines;
        vector<CombatSportsWorkoutRoundInput> rounds;
        if (!parseWorkoutFields(
                body,
                name,
                description,
                disciplines,
                rounds,
                error)) {
            return createWorkoutErrorResponse(400, error);
        }

        DatabaseResult result =
            database.updateCombatSportsWorkoutTemplate(
                workoutId,
                userId,
                name,
                description,
                disciplines,
                rounds
            );

        if (result != DatabaseResult::Success) {
            return workoutDatabaseFailure(result, "update");
        }

        CombatSportsWorkoutTemplate workout;
        result = database.getCombatSportsWorkoutTemplate(
            workoutId,
            userId,
            workout
        );
        if (result != DatabaseResult::Success) {
            return createWorkoutErrorResponse(
                500,
                "Workout was updated but could not be retrieved."
            );
        }

        return workoutSuccessResponse(
            200,
            "Workout template updated successfully.",
            workout
        );
    });

    CROW_ROUTE(app, "/api/combat-sports/workouts/<int>/duplicate")
    .methods(crow::HTTPMethod::POST)
    ([&database](const crow::request& request, int workoutId) {
        int userId;
        crow::response authenticationFailure;
        if (!authenticateWorkoutRequest(
                request,
                database,
                userId,
                authenticationFailure)) {
            return authenticationFailure;
        }
        if (workoutId <= 0) {
            return createWorkoutErrorResponse(
                400,
                "Workout ID must be greater than zero."
            );
        }

        auto body = crow::json::load(request.body);
        if (!body ||
            body.t() != crow::json::type::Object ||
            !body.has("name") ||
            body["name"].t() != crow::json::type::String) {
            return createWorkoutErrorResponse(
                400,
                "A new workout name is required."
            );
        }

        string duplicatedName = body["name"].s();
        if (isBlank(duplicatedName)) {
            return createWorkoutErrorResponse(
                400,
                "Workout name cannot be empty."
            );
        }

        int duplicatedId = 0;
        DatabaseResult result =
            database.duplicateCombatSportsWorkoutTemplate(
                workoutId,
                userId,
                duplicatedName,
                duplicatedId
            );

        if (result != DatabaseResult::Success) {
            return workoutDatabaseFailure(result, "duplicate");
        }

        CombatSportsWorkoutTemplate duplicate;
        result = database.getCombatSportsWorkoutTemplate(
            duplicatedId,
            userId,
            duplicate
        );
        if (result != DatabaseResult::Success) {
            return createWorkoutErrorResponse(
                500,
                "Workout was duplicated but could not be retrieved."
            );
        }

        return workoutSuccessResponse(
            201,
            "Workout template duplicated successfully.",
            duplicate
        );
    });

    CROW_ROUTE(app, "/api/combat-sports/workouts/<int>")
    .methods(crow::HTTPMethod::Delete)
    ([&database](const crow::request& request, int workoutId) {
        int userId;
        crow::response authenticationFailure;
        if (!authenticateWorkoutRequest(
                request,
                database,
                userId,
                authenticationFailure)) {
            return authenticationFailure;
        }
        if (workoutId <= 0) {
            return createWorkoutErrorResponse(
                400,
                "Workout ID must be greater than zero."
            );
        }

        DatabaseResult result =
            database.deleteCombatSportsWorkoutTemplate(
                workoutId,
                userId
            );

        if (result != DatabaseResult::Success) {
            return workoutDatabaseFailure(result, "delete");
        }

        crow::json::wvalue body;
        body["success"] = true;
        body["message"] = "Workout template deleted successfully.";
        body["deleted_workout_id"] = workoutId;

        crow::response response(200, body);
        response.add_header("Content-Type", "application/json");
        return response;
    });
}
