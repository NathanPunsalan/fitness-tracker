#include "combat_sports_content_routes.h"

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

// Creates a standardized JSON response for successful API requests.
crow::response createContentSuccessResponse(
    int statusCode,
    const string& message
) {
    crow::json::wvalue body;
    body["success"] = true;
    body["message"] = message;

    crow::response response(statusCode, body);
    response.add_header("Content-Type", "application/json");
    return response;
}

// Creates a standardized JSON response for failed API requests.
crow::response createContentErrorResponse(
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

// Authenticates a content request and builds the correct failure response.
bool authenticateContentRequest(
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
        failureResponse = createContentErrorResponse(
            401,
            authenticationError
        );
        return false;
    }

    if (result == AuthenticationResult::Error) {
        failureResponse = createContentErrorResponse(
            500,
            authenticationError
        );
        return false;
    }

    return true;
}

crow::json::wvalue techniqueToJson(
    const CombatSportsTechnique& technique
) {
    crow::json::wvalue json;
    json["id"] = technique.id;
    json["discipline"] = technique.discipline;
    json["name"] = technique.name;
    json["category"] = technique.category;
    json["description"] = technique.description;
    json["created_at"] = technique.createdAt;
    json["updated_at"] = technique.updatedAt;
    return json;
}

crow::json::wvalue combinationToJson(
    const CombatSportsCombination& combination
) {
    crow::json::wvalue json;
    json["id"] = combination.id;
    json["discipline"] = combination.discipline;
    json["name"] = combination.name;
    json["description"] = combination.description;
    json["created_at"] = combination.createdAt;
    json["updated_at"] = combination.updatedAt;

    crow::json::wvalue::list steps;

    for (const CombatSportsCombinationStep& step : combination.steps) {
        crow::json::wvalue stepJson;
        stepJson["id"] = step.id;
        stepJson["technique_id"] = step.techniqueId;
        stepJson["step_order"] = step.stepOrder;
        stepJson["technique_name"] = step.techniqueName;
        stepJson["technique_category"] = step.techniqueCategory;
        steps.push_back(move(stepJson));
    }

    json["steps"] = move(steps);
    return json;
}

crow::json::wvalue drillToJson(const CombatSportsDrill& drill) {
    crow::json::wvalue json;
    json["id"] = drill.id;
    json["discipline"] = drill.discipline;
    json["name"] = drill.name;
    json["instructions"] = drill.instructions;
    json["notes"] = drill.notes;
    json["created_at"] = drill.createdAt;
    json["updated_at"] = drill.updatedAt;

    if (drill.defaultDurationSeconds.has_value()) {
        json["default_duration_seconds"] =
            drill.defaultDurationSeconds.value();
    }
    else {
        json["default_duration_seconds"] = nullptr;
    }

    if (drill.defaultRepetitions.has_value()) {
        json["default_repetitions"] =
            drill.defaultRepetitions.value();
    }
    else {
        json["default_repetitions"] = nullptr;
    }

    if (drill.defaultRounds.has_value()) {
        json["default_rounds"] = drill.defaultRounds.value();
    }
    else {
        json["default_rounds"] = nullptr;
    }

    crow::json::wvalue::list items;

    for (const CombatSportsDrillItem& item : drill.items) {
        crow::json::wvalue itemJson;
        itemJson["id"] = item.id;
        itemJson["item_type"] = item.itemType;
        itemJson["item_order"] = item.itemOrder;
        itemJson["item_name"] = item.itemName;

        if (item.techniqueId.has_value()) {
            itemJson["technique_id"] = item.techniqueId.value();
        }
        else {
            itemJson["technique_id"] = nullptr;
        }

        if (item.combinationId.has_value()) {
            itemJson["combination_id"] = item.combinationId.value();
        }
        else {
            itemJson["combination_id"] = nullptr;
        }

        items.push_back(move(itemJson));
    }

    json["items"] = move(items);
    return json;
}

bool parseTechniqueFields(
    const crow::json::rvalue& body,
    string& discipline,
    string& name,
    string& category,
    string& description,
    string& error
) {
    if (!body.has("discipline") || !body.has("name")) {
        error = "Discipline and technique name are required.";
        return false;
    }

    if (body["discipline"].t() != crow::json::type::String ||
        body["name"].t() != crow::json::type::String) {
        error = "Discipline and technique name must be text values.";
        return false;
    }

    if (body.has("category") &&
        body["category"].t() != crow::json::type::String) {
        error = "Technique category must be a text value.";
        return false;
    }

    if (body.has("description") &&
        body["description"].t() != crow::json::type::String) {
        error = "Technique description must be a text value.";
        return false;
    }

    discipline = body["discipline"].s();
    name = body["name"].s();
    category = body.has("category") ? string(body["category"].s()) : "";
    description = body.has("description")
        ? string(body["description"].s())
        : "";

    if (isBlank(discipline) || isBlank(name)) {
        error = "Discipline and technique name cannot be empty.";
        return false;
    }

    return true;
}

bool parseCombinationFields(
    const crow::json::rvalue& body,
    string& discipline,
    string& name,
    string& description,
    vector<int>& techniqueIds,
    string& error
) {
    if (!body.has("discipline") ||
        !body.has("name") ||
        !body.has("technique_ids")) {
        error = "Discipline, combination name, and technique IDs are required.";
        return false;
    }

    if (body["discipline"].t() != crow::json::type::String ||
        body["name"].t() != crow::json::type::String ||
        body["technique_ids"].t() != crow::json::type::List) {
        error = "Combination fields have invalid data types.";
        return false;
    }

    if (body.has("description") &&
        body["description"].t() != crow::json::type::String) {
        error = "Combination description must be a text value.";
        return false;
    }

    discipline = body["discipline"].s();
    name = body["name"].s();
    description = body.has("description")
        ? string(body["description"].s())
        : "";

    if (isBlank(discipline) || isBlank(name)) {
        error = "Discipline and combination name cannot be empty.";
        return false;
    }

    techniqueIds.clear();

    for (const crow::json::rvalue& item : body["technique_ids"]) {
        if (item.t() != crow::json::type::Number ||
            item.i() <= 0 ||
            item.i() > numeric_limits<int>::max()) {
            error = "Every technique ID must be a positive number.";
            return false;
        }

        techniqueIds.push_back(static_cast<int>(item.i()));
    }

    if (techniqueIds.empty()) {
        error = "A combination must contain at least one technique.";
        return false;
    }

    return true;
}

bool parseOptionalPositiveInteger(
    const crow::json::rvalue& body,
    const string& fieldName,
    optional<int>& output,
    string& error
) {
    if (!body.has(fieldName.c_str())) {
        output.reset();
        return true;
    }

    const crow::json::rvalue& value = body[fieldName.c_str()];

    if (value.t() == crow::json::type::Null) {
        output.reset();
        return true;
    }

    if (value.t() != crow::json::type::Number ||
        value.i() <= 0 ||
        value.i() > numeric_limits<int>::max()) {
        error = fieldName + " must be a positive number or null.";
        return false;
    }

    // Crow exposes JSON integers as 64-bit values. The upper-bound check
    // above makes this conversion to the database's int type safe.
    output = static_cast<int>(value.i());
    return true;
}

bool parseDrillFields(
    const crow::json::rvalue& body,
    string& discipline,
    string& name,
    string& instructions,
    optional<int>& defaultDurationSeconds,
    optional<int>& defaultRepetitions,
    optional<int>& defaultRounds,
    string& notes,
    vector<CombatSportsDrillItemInput>& items,
    string& error
) {
    if (!body.has("discipline") ||
        !body.has("name") ||
        !body.has("items")) {
        error = "Discipline, drill name, and drill items are required.";
        return false;
    }

    if (body["discipline"].t() != crow::json::type::String ||
        body["name"].t() != crow::json::type::String ||
        body["items"].t() != crow::json::type::List) {
        error = "Drill fields have invalid data types.";
        return false;
    }

    if (body.has("instructions") &&
        body["instructions"].t() != crow::json::type::String) {
        error = "Drill instructions must be a text value.";
        return false;
    }

    if (body.has("notes") &&
        body["notes"].t() != crow::json::type::String) {
        error = "Drill notes must be a text value.";
        return false;
    }

    discipline = body["discipline"].s();
    name = body["name"].s();
    instructions = body.has("instructions")
        ? string(body["instructions"].s())
        : "";
    notes = body.has("notes") ? string(body["notes"].s()) : "";

    if (isBlank(discipline) || isBlank(name)) {
        error = "Discipline and drill name cannot be empty.";
        return false;
    }

    if (!parseOptionalPositiveInteger(
            body,
            "default_duration_seconds",
            defaultDurationSeconds,
            error) ||
        !parseOptionalPositiveInteger(
            body,
            "default_repetitions",
            defaultRepetitions,
            error) ||
        !parseOptionalPositiveInteger(
            body,
            "default_rounds",
            defaultRounds,
            error)) {
        return false;
    }

    items.clear();

    for (const crow::json::rvalue& item : body["items"]) {
        if (item.t() != crow::json::type::Object ||
            !item.has("item_type") ||
            !item.has("reference_id")) {
            error = "Each drill item requires an item type and reference ID.";
            return false;
        }

        if (item["item_type"].t() != crow::json::type::String ||
            item["reference_id"].t() != crow::json::type::Number) {
            error = "Drill item type must be text and reference ID must be a number.";
            return false;
        }

        string itemType = item["item_type"].s();
        const int64_t referenceIdValue = item["reference_id"].i();

        if ((itemType != "technique" && itemType != "combination") ||
            referenceIdValue <= 0 ||
            referenceIdValue > numeric_limits<int>::max()) {
            error = "Drill items must reference a valid technique or combination.";
            return false;
        }

        items.push_back({
            itemType,
            static_cast<int>(referenceIdValue)
        });
    }

    if (items.empty()) {
        error = "A drill must contain at least one item.";
        return false;
    }

    return true;
}

crow::response techniqueDatabaseFailure(
    DatabaseResult result,
    const string& action
) {
    if (result == DatabaseResult::NotFound) {
        return createContentErrorResponse(404, "Combat-sports technique not found.");
    }
    if (result == DatabaseResult::Conflict) {
        return createContentErrorResponse(
            409,
            "Technique name already exists or the technique is still referenced."
        );
    }
    return createContentErrorResponse(500, "Unable to " + action + " combat-sports technique.");
}

crow::response combinationDatabaseFailure(
    DatabaseResult result,
    const string& action
) {
    if (result == DatabaseResult::NotFound) {
        return createContentErrorResponse(
            404,
            "Combination or referenced technique not found."
        );
    }
    if (result == DatabaseResult::Conflict) {
        return createContentErrorResponse(
            409,
            "Combination name already exists or the combination is still referenced."
        );
    }
    return createContentErrorResponse(500, "Unable to " + action + " combat-sports combination.");
}

crow::response drillDatabaseFailure(
    DatabaseResult result,
    const string& action
) {
    if (result == DatabaseResult::NotFound) {
        return createContentErrorResponse(
            404,
            "Drill or referenced content not found."
        );
    }
    if (result == DatabaseResult::Conflict) {
        return createContentErrorResponse(
            409,
            "Drill name already exists or the drill contains unsupported values."
        );
    }
    return createContentErrorResponse(500, "Unable to " + action + " combat-sports drill.");
}

} // namespace

void registerCombatSportsContentRoutes(
    crow::SimpleApp& app,
    Database& database
) {
    CROW_ROUTE(app, "/api/combat-sports/techniques")
    .methods(crow::HTTPMethod::POST)
    ([&database](const crow::request& request) {
        int userId;
        crow::response authenticationFailure;
        if (!authenticateContentRequest(request, database, userId, authenticationFailure)) {
            return authenticationFailure;
        }

        auto body = crow::json::load(request.body);
        if (!body) {
            return createContentErrorResponse(400, "Invalid technique data.");
        }

        string discipline, name, category, description, error;
        if (!parseTechniqueFields(body, discipline, name, category, description, error)) {
            return createContentErrorResponse(400, error);
        }

        int techniqueId = 0;
        DatabaseResult result = database.createCombatSportsTechnique(
            userId, discipline, name, category, description, techniqueId
        );
        if (result != DatabaseResult::Success) {
            return techniqueDatabaseFailure(result, "create");
        }

        CombatSportsTechnique technique;
        result = database.getCombatSportsTechnique(techniqueId, userId, technique);
        if (result != DatabaseResult::Success) {
            return createContentErrorResponse(500, "Technique was created but could not be retrieved.");
        }

        crow::json::wvalue responseBody;
        responseBody["success"] = true;
        responseBody["message"] = "Combat-sports technique created successfully.";
        responseBody["technique"] = techniqueToJson(technique);
        crow::response response(201, responseBody);
        response.add_header("Content-Type", "application/json");
        return response;
    });

    CROW_ROUTE(app, "/api/combat-sports/techniques")
    .methods(crow::HTTPMethod::GET)
    ([&database](const crow::request& request) {
        int userId;
        crow::response authenticationFailure;
        if (!authenticateContentRequest(request, database, userId, authenticationFailure)) {
            return authenticationFailure;
        }

        vector<CombatSportsTechnique> techniques;
        DatabaseResult result = database.getCombatSportsTechniques(userId, techniques);
        if (result != DatabaseResult::Success) {
            return techniqueDatabaseFailure(result, "retrieve");
        }

        crow::json::wvalue::list list;
        for (const CombatSportsTechnique& technique : techniques) {
            list.push_back(techniqueToJson(technique));
        }

        crow::json::wvalue responseBody;
        responseBody["success"] = true;
        responseBody["techniques"] = move(list);
        responseBody["count"] = static_cast<int>(techniques.size());
        crow::response response(200, responseBody);
        response.add_header("Content-Type", "application/json");
        return response;
    });

    CROW_ROUTE(app, "/api/combat-sports/techniques/<int>")
    .methods(crow::HTTPMethod::GET)
    ([&database](const crow::request& request, int techniqueId) {
        int userId;
        crow::response authenticationFailure;
        if (!authenticateContentRequest(request, database, userId, authenticationFailure)) {
            return authenticationFailure;
        }
        if (techniqueId <= 0) {
            return createContentErrorResponse(400, "Technique ID must be greater than zero.");
        }

        CombatSportsTechnique technique;
        DatabaseResult result = database.getCombatSportsTechnique(techniqueId, userId, technique);
        if (result != DatabaseResult::Success) {
            return techniqueDatabaseFailure(result, "retrieve");
        }

        crow::json::wvalue responseBody;
        responseBody["success"] = true;
        responseBody["technique"] = techniqueToJson(technique);
        crow::response response(200, responseBody);
        response.add_header("Content-Type", "application/json");
        return response;
    });

    CROW_ROUTE(app, "/api/combat-sports/techniques/<int>")
    .methods(crow::HTTPMethod::PUT)
    ([&database](const crow::request& request, int techniqueId) {
        int userId;
        crow::response authenticationFailure;
        if (!authenticateContentRequest(request, database, userId, authenticationFailure)) {
            return authenticationFailure;
        }
        if (techniqueId <= 0) {
            return createContentErrorResponse(400, "Technique ID must be greater than zero.");
        }

        auto body = crow::json::load(request.body);
        if (!body) {
            return createContentErrorResponse(400, "Invalid technique data.");
        }

        string discipline, name, category, description, error;
        if (!parseTechniqueFields(body, discipline, name, category, description, error)) {
            return createContentErrorResponse(400, error);
        }

        DatabaseResult result = database.updateCombatSportsTechnique(
            techniqueId, userId, discipline, name, category, description
        );
        if (result != DatabaseResult::Success) {
            return techniqueDatabaseFailure(result, "update");
        }

        CombatSportsTechnique technique;
        result = database.getCombatSportsTechnique(techniqueId, userId, technique);
        if (result != DatabaseResult::Success) {
            return createContentErrorResponse(500, "Technique was updated but could not be retrieved.");
        }

        crow::json::wvalue responseBody;
        responseBody["success"] = true;
        responseBody["message"] = "Combat-sports technique updated successfully.";
        responseBody["technique"] = techniqueToJson(technique);
        crow::response response(200, responseBody);
        response.add_header("Content-Type", "application/json");
        return response;
    });

    CROW_ROUTE(app, "/api/combat-sports/techniques/<int>")
    .methods(crow::HTTPMethod::Delete)
    ([&database](const crow::request& request, int techniqueId) {
        int userId;
        crow::response authenticationFailure;
        if (!authenticateContentRequest(request, database, userId, authenticationFailure)) {
            return authenticationFailure;
        }
        if (techniqueId <= 0) {
            return createContentErrorResponse(400, "Technique ID must be greater than zero.");
        }

        DatabaseResult result = database.deleteCombatSportsTechnique(techniqueId, userId);
        if (result != DatabaseResult::Success) {
            return techniqueDatabaseFailure(result, "delete");
        }

        crow::json::wvalue responseBody;
        responseBody["success"] = true;
        responseBody["message"] = "Combat-sports technique deleted successfully.";
        responseBody["deleted_technique_id"] = techniqueId;
        crow::response response(200, responseBody);
        response.add_header("Content-Type", "application/json");
        return response;
    });

    CROW_ROUTE(app, "/api/combat-sports/combinations")
    .methods(crow::HTTPMethod::POST)
    ([&database](const crow::request& request) {
        int userId;
        crow::response authenticationFailure;
        if (!authenticateContentRequest(request, database, userId, authenticationFailure)) {
            return authenticationFailure;
        }
        auto body = crow::json::load(request.body);
        if (!body) {
            return createContentErrorResponse(400, "Invalid combination data.");
        }

        string discipline, name, description, error;
        vector<int> techniqueIds;
        if (!parseCombinationFields(body, discipline, name, description, techniqueIds, error)) {
            return createContentErrorResponse(400, error);
        }

        int combinationId = 0;
        DatabaseResult result = database.createCombatSportsCombination(
            userId, discipline, name, description, techniqueIds, combinationId
        );
        if (result != DatabaseResult::Success) {
            return combinationDatabaseFailure(result, "create");
        }

        CombatSportsCombination combination;
        result = database.getCombatSportsCombination(combinationId, userId, combination);
        if (result != DatabaseResult::Success) {
            return createContentErrorResponse(500, "Combination was created but could not be retrieved.");
        }

        crow::json::wvalue responseBody;
        responseBody["success"] = true;
        responseBody["message"] = "Combat-sports combination created successfully.";
        responseBody["combination"] = combinationToJson(combination);
        crow::response response(201, responseBody);
        response.add_header("Content-Type", "application/json");
        return response;
    });

    CROW_ROUTE(app, "/api/combat-sports/combinations")
    .methods(crow::HTTPMethod::GET)
    ([&database](const crow::request& request) {
        int userId;
        crow::response authenticationFailure;
        if (!authenticateContentRequest(request, database, userId, authenticationFailure)) {
            return authenticationFailure;
        }
        vector<CombatSportsCombination> combinations;
        DatabaseResult result = database.getCombatSportsCombinations(userId, combinations);
        if (result != DatabaseResult::Success) {
            return combinationDatabaseFailure(result, "retrieve");
        }

        crow::json::wvalue::list list;
        for (const CombatSportsCombination& combination : combinations) {
            list.push_back(combinationToJson(combination));
        }
        crow::json::wvalue responseBody;
        responseBody["success"] = true;
        responseBody["combinations"] = move(list);
        responseBody["count"] = static_cast<int>(combinations.size());
        crow::response response(200, responseBody);
        response.add_header("Content-Type", "application/json");
        return response;
    });

    CROW_ROUTE(app, "/api/combat-sports/combinations/<int>")
    .methods(crow::HTTPMethod::GET)
    ([&database](const crow::request& request, int combinationId) {
        int userId;
        crow::response authenticationFailure;
        if (!authenticateContentRequest(request, database, userId, authenticationFailure)) {
            return authenticationFailure;
        }
        if (combinationId <= 0) {
            return createContentErrorResponse(400, "Combination ID must be greater than zero.");
        }
        CombatSportsCombination combination;
        DatabaseResult result = database.getCombatSportsCombination(combinationId, userId, combination);
        if (result != DatabaseResult::Success) {
            return combinationDatabaseFailure(result, "retrieve");
        }
        crow::json::wvalue responseBody;
        responseBody["success"] = true;
        responseBody["combination"] = combinationToJson(combination);
        crow::response response(200, responseBody);
        response.add_header("Content-Type", "application/json");
        return response;
    });

    CROW_ROUTE(app, "/api/combat-sports/combinations/<int>")
    .methods(crow::HTTPMethod::PUT)
    ([&database](const crow::request& request, int combinationId) {
        int userId;
        crow::response authenticationFailure;
        if (!authenticateContentRequest(request, database, userId, authenticationFailure)) {
            return authenticationFailure;
        }
        if (combinationId <= 0) {
            return createContentErrorResponse(400, "Combination ID must be greater than zero.");
        }
        auto body = crow::json::load(request.body);
        if (!body) {
            return createContentErrorResponse(400, "Invalid combination data.");
        }
        string discipline, name, description, error;
        vector<int> techniqueIds;
        if (!parseCombinationFields(body, discipline, name, description, techniqueIds, error)) {
            return createContentErrorResponse(400, error);
        }
        DatabaseResult result = database.updateCombatSportsCombination(
            combinationId, userId, discipline, name, description, techniqueIds
        );
        if (result != DatabaseResult::Success) {
            return combinationDatabaseFailure(result, "update");
        }
        CombatSportsCombination combination;
        result = database.getCombatSportsCombination(combinationId, userId, combination);
        if (result != DatabaseResult::Success) {
            return createContentErrorResponse(500, "Combination was updated but could not be retrieved.");
        }
        crow::json::wvalue responseBody;
        responseBody["success"] = true;
        responseBody["message"] = "Combat-sports combination updated successfully.";
        responseBody["combination"] = combinationToJson(combination);
        crow::response response(200, responseBody);
        response.add_header("Content-Type", "application/json");
        return response;
    });

    CROW_ROUTE(app, "/api/combat-sports/combinations/<int>")
    .methods(crow::HTTPMethod::Delete)
    ([&database](const crow::request& request, int combinationId) {
        int userId;
        crow::response authenticationFailure;
        if (!authenticateContentRequest(request, database, userId, authenticationFailure)) {
            return authenticationFailure;
        }
        if (combinationId <= 0) {
            return createContentErrorResponse(400, "Combination ID must be greater than zero.");
        }
        DatabaseResult result = database.deleteCombatSportsCombination(combinationId, userId);
        if (result != DatabaseResult::Success) {
            return combinationDatabaseFailure(result, "delete");
        }
        crow::json::wvalue responseBody;
        responseBody["success"] = true;
        responseBody["message"] = "Combat-sports combination deleted successfully.";
        responseBody["deleted_combination_id"] = combinationId;
        crow::response response(200, responseBody);
        response.add_header("Content-Type", "application/json");
        return response;
    });

    CROW_ROUTE(app, "/api/combat-sports/drills")
    .methods(crow::HTTPMethod::POST)
    ([&database](const crow::request& request) {
        int userId;
        crow::response authenticationFailure;
        if (!authenticateContentRequest(request, database, userId, authenticationFailure)) {
            return authenticationFailure;
        }
        auto body = crow::json::load(request.body);
        if (!body) {
            return createContentErrorResponse(400, "Invalid drill data.");
        }
        string discipline, name, instructions, notes, error;
        optional<int> duration, repetitions, rounds;
        vector<CombatSportsDrillItemInput> items;
        if (!parseDrillFields(body, discipline, name, instructions, duration, repetitions, rounds, notes, items, error)) {
            return createContentErrorResponse(400, error);
        }
        int drillId = 0;
        DatabaseResult result = database.createCombatSportsDrill(
            userId, discipline, name, instructions, duration,
            repetitions, rounds, notes, items, drillId
        );
        if (result != DatabaseResult::Success) {
            return drillDatabaseFailure(result, "create");
        }
        CombatSportsDrill drill;
        result = database.getCombatSportsDrill(drillId, userId, drill);
        if (result != DatabaseResult::Success) {
            return createContentErrorResponse(500, "Drill was created but could not be retrieved.");
        }
        crow::json::wvalue responseBody;
        responseBody["success"] = true;
        responseBody["message"] = "Combat-sports drill created successfully.";
        responseBody["drill"] = drillToJson(drill);
        crow::response response(201, responseBody);
        response.add_header("Content-Type", "application/json");
        return response;
    });

    CROW_ROUTE(app, "/api/combat-sports/drills")
    .methods(crow::HTTPMethod::GET)
    ([&database](const crow::request& request) {
        int userId;
        crow::response authenticationFailure;
        if (!authenticateContentRequest(request, database, userId, authenticationFailure)) {
            return authenticationFailure;
        }
        vector<CombatSportsDrill> drills;
        DatabaseResult result = database.getCombatSportsDrills(userId, drills);
        if (result != DatabaseResult::Success) {
            return drillDatabaseFailure(result, "retrieve");
        }
        crow::json::wvalue::list list;
        for (const CombatSportsDrill& drill : drills) {
            list.push_back(drillToJson(drill));
        }
        crow::json::wvalue responseBody;
        responseBody["success"] = true;
        responseBody["drills"] = move(list);
        responseBody["count"] = static_cast<int>(drills.size());
        crow::response response(200, responseBody);
        response.add_header("Content-Type", "application/json");
        return response;
    });

    CROW_ROUTE(app, "/api/combat-sports/drills/<int>")
    .methods(crow::HTTPMethod::GET)
    ([&database](const crow::request& request, int drillId) {
        int userId;
        crow::response authenticationFailure;
        if (!authenticateContentRequest(request, database, userId, authenticationFailure)) {
            return authenticationFailure;
        }
        if (drillId <= 0) {
            return createContentErrorResponse(400, "Drill ID must be greater than zero.");
        }
        CombatSportsDrill drill;
        DatabaseResult result = database.getCombatSportsDrill(drillId, userId, drill);
        if (result != DatabaseResult::Success) {
            return drillDatabaseFailure(result, "retrieve");
        }
        crow::json::wvalue responseBody;
        responseBody["success"] = true;
        responseBody["drill"] = drillToJson(drill);
        crow::response response(200, responseBody);
        response.add_header("Content-Type", "application/json");
        return response;
    });

    CROW_ROUTE(app, "/api/combat-sports/drills/<int>")
    .methods(crow::HTTPMethod::PUT)
    ([&database](const crow::request& request, int drillId) {
        int userId;
        crow::response authenticationFailure;
        if (!authenticateContentRequest(request, database, userId, authenticationFailure)) {
            return authenticationFailure;
        }
        if (drillId <= 0) {
            return createContentErrorResponse(400, "Drill ID must be greater than zero.");
        }
        auto body = crow::json::load(request.body);
        if (!body) {
            return createContentErrorResponse(400, "Invalid drill data.");
        }
        string discipline, name, instructions, notes, error;
        optional<int> duration, repetitions, rounds;
        vector<CombatSportsDrillItemInput> items;
        if (!parseDrillFields(body, discipline, name, instructions, duration, repetitions, rounds, notes, items, error)) {
            return createContentErrorResponse(400, error);
        }
        DatabaseResult result = database.updateCombatSportsDrill(
            drillId, userId, discipline, name, instructions, duration,
            repetitions, rounds, notes, items
        );
        if (result != DatabaseResult::Success) {
            return drillDatabaseFailure(result, "update");
        }
        CombatSportsDrill drill;
        result = database.getCombatSportsDrill(drillId, userId, drill);
        if (result != DatabaseResult::Success) {
            return createContentErrorResponse(500, "Drill was updated but could not be retrieved.");
        }
        crow::json::wvalue responseBody;
        responseBody["success"] = true;
        responseBody["message"] = "Combat-sports drill updated successfully.";
        responseBody["drill"] = drillToJson(drill);
        crow::response response(200, responseBody);
        response.add_header("Content-Type", "application/json");
        return response;
    });

    CROW_ROUTE(app, "/api/combat-sports/drills/<int>")
    .methods(crow::HTTPMethod::Delete)
    ([&database](const crow::request& request, int drillId) {
        int userId;
        crow::response authenticationFailure;
        if (!authenticateContentRequest(request, database, userId, authenticationFailure)) {
            return authenticationFailure;
        }
        if (drillId <= 0) {
            return createContentErrorResponse(400, "Drill ID must be greater than zero.");
        }
        DatabaseResult result = database.deleteCombatSportsDrill(drillId, userId);
        if (result != DatabaseResult::Success) {
            return drillDatabaseFailure(result, "delete");
        }
        crow::json::wvalue responseBody;
        responseBody["success"] = true;
        responseBody["message"] = "Combat-sports drill deleted successfully.";
        responseBody["deleted_drill_id"] = drillId;
        crow::response response(200, responseBody);
        response.add_header("Content-Type", "application/json");
        return response;
    });
}
