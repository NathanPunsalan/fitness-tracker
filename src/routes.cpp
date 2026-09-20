#include "routes.h"
#include "auth.h"
#include "validation.h"

#include <string>
#include <utility>
#include <vector>

using namespace std;

// Creates a standardized JSON response for successful API requests
crow::response createJsonSuccessResponse(
    int statusCode,
    const string& message
) {
    crow::json::wvalue responseBody;

    responseBody["success"] = true;
    responseBody["message"] = message;

    crow::response response(
        statusCode,
        responseBody
    );

    response.add_header(
        "Content-Type",
        "application/json"
    );

    return response;
}

// Creates a standardized JSON response for failed API requests
crow::response createJsonErrorResponse(
    int statusCode,
    const string& error
) {
    crow::json::wvalue responseBody;

    responseBody["success"] = false;
    responseBody["error"] = error;

    crow::response response(
        statusCode,
        responseBody
    );

    response.add_header(
        "Content-Type",
        "application/json"
    );

    return response;
}

// Creates the navigation menu shared by the application's main pages
string createNavigation()
{
    return R"(
        <nav>
            <a href="/">Dashboard</a> |
            <a href="/lifting">Lifting</a> |
            <a href="/running">Running</a> |
            <a href="/nutrition">Nutrition</a> |
            <a href="/account">Account</a>
        </nav>
        <hr>
        )";
}

// Creates a placeholder page with the shared nav menu
string createPage(const string& title, const string& message)
{
    return R"(
        <!DOCTYPE html>
        <html>
        <head>
            <title>)" + title + R"(</title>
        </head>
        <body>
    )"
        + createNavigation()
        + "<h1>" + title + "</h1>"
        + "<p>" + message + "</p>"
        + R"(
        </body>
        </html>
    )";
}

// Registers the application's basic routes
void registerRoutes(crow::SimpleApp& app, Database& database)
{
    // Home route used to confirm Fitness Tracker app is running
    CROW_ROUTE(app, "/")([]() {
        return crow::response(
            200,
            createPage(
                "Fitness Tracker",
                "Fitness Tracker is running."
            )
        );
    });

    // Lifting placeholder route
    CROW_ROUTE(app, "/lifting")
    ([&database](const crow::request& request) {
        
        int userId;
        string authenticationError;

        AuthenticationResult authenticationResult = authenticateRequest(
            request,
            database,
            userId,
            authenticationError
        );

        // The request does not contain a valid authenticated session
        if (authenticationResult == AuthenticationResult::Unauthorized) {
            return crow::response(
                401,
                authenticationError
            );
        }

        // An internal error prevented session validation
        if (authenticationResult == AuthenticationResult::Error) {
            return crow::response(
                500,
                authenticationError
            );
        }

        return crow::response(
            200,
            createPage(
                "Lifting",
                "Lifting features will be added in a future update."
            )
        );
    });

    // Running placeholder route
    CROW_ROUTE(app, "/running")
    ([&database](const crow::request& request) {

        int userId;
        string authenticationError;

        AuthenticationResult authenticationResult = authenticateRequest(
            request,
            database,
            userId,
            authenticationError
        );

        // The request does not contain a valid authenticated session
        if (authenticationResult == AuthenticationResult::Unauthorized) {
            return crow::response(
                401,
                authenticationError
            );
        }

        // An internal error prevented session validation
        if (authenticationResult == AuthenticationResult::Error) {
            return crow::response(
                500,
                authenticationError
            );
        }

        return crow::response(
            200,
            createPage(
                "Running",
                "Running features will be added in a future update."
            )
        );
    });

    // Nutrition placeholder route
    CROW_ROUTE(app, "/nutrition")
    ([&database](const crow::request& request) {

        int userId;
        string authenticationError;

        AuthenticationResult authenticationResult = authenticateRequest(
            request,
            database,
            userId,
            authenticationError
        );

        // The request does not contain a valid authenticated session
        if (authenticationResult == AuthenticationResult::Unauthorized) {
            return crow::response(
                401,
                authenticationError
            );
        }

        // An internal error prevented session validation
        if (authenticationResult == AuthenticationResult::Error) {
            return crow::response(
                500,
                authenticationError
            );
        }

        return crow::response(
            200,
            createPage(
                "Nutrition",
                "Nutrition features will be added in a future update."
            )
        );
    });

    // Account route protected by session authentication
    CROW_ROUTE(app, "/account")
    ([&database](const crow::request& request) {

        int userId;
        string authenticationError;

        AuthenticationResult authenticationResult = authenticateRequest(
            request,
            database,
            userId,
            authenticationError
        );

        // The request does not contain a valid authenticated session
        if (authenticationResult == AuthenticationResult::Unauthorized) {
            return crow::response(
                401,
                authenticationError
            );
        }

        // An internal error prevented session validation
        if (authenticationResult == AuthenticationResult::Error) {
            return crow::response(
                500,
                authenticationError
            );
        }

        // The request is authenticated, so the protected page can be displayed
        return crow::response(
            200,
            createPage(
                "Account",
                "Account features will be added in a future update."
            )
        );
    });

    // Creates a new user account
    CROW_ROUTE(app, "/api/register").methods(crow::HTTPMethod::POST)
    ([&database](const crow::request& request) {

        // Parse JSON body sent with registration request
        auto body = crow::json::load(request.body);

        // Make sure request contains valid JSON
        if (!body) {
            return createJsonErrorResponse(
                400,
                "Invalid registration data."
            );
        }

        // Make sure all required fields are present
        if (!body.has("username") ||
            !body.has("email") ||
            !body.has("password")) {
                
                return createJsonErrorResponse(
                    400,
                    "Username, email, and password are required."
                );
        }
        
        // Make sure all registration fields contain string values
        if (body["username"].t() != crow::json::type::String ||
            body["email"].t() != crow::json::type::String ||
            body["password"].t() != crow::json::type::String) {

                return createJsonErrorResponse(
                    400,
                    "Username, email, and password must be text values."
                );
            }

        // Read registration fields from the request
        string username = body["username"].s();
        string email = body["email"].s();
        string password = body["password"].s();

        // Reject empty registration fields
        if (isBlank(username) || isBlank(email) || isBlank(password)) {
            return createJsonErrorResponse(
                400,
                "Username, email, and password cannot be empty."
            );
        }

        // Validate username requirements
        if (!isValidUsername(username)) {
            return createJsonErrorResponse(
                400,
                "Username must be between 5 and 15 characters."
            );
        }

        // Validate email format
        if (!isValidEmail(email)) {
            return createJsonErrorResponse(
                400,
                "Please enter a valid email address."
            );
        }

        // Validate password security requirements
        if (!isValidPassword(password)) {
            return createJsonErrorResponse(
                400,
                "Password must be at least 8 characters and include an uppercase letter, lowercase letter, number, and symbol."
            );
        }

        // Hash the password before storing it in the database
        string passwordHash = hashPassword(password);

        // Attempt to create user account
        DatabaseResult result = database.createUser(
            username,
            email,
            passwordHash
        );

        // A username or email with the submitted value already exists
        if (result == DatabaseResult::Conflict) {
            return createJsonErrorResponse(
                409,
                "Username or email already exists."
            );
        }

        // An unexpected database error prevented account creation
        if (result == DatabaseResult::Error) {
            return createJsonErrorResponse(
                500,
                "Unable to create user account."
            );
        }

        // The Account was successfully created
        return createJsonSuccessResponse(
            201,
            "User account created successfully."
        );
    });

    CROW_ROUTE(app, "/api/login").methods(crow::HTTPMethod::POST)
    ([&database](const crow::request& request) {

        // Parse JSON body sent with the login request
        auto body = crow::json::load(request.body);

        // Ensures the request contains valid JSON
        if (!body) {
            return createJsonErrorResponse(
                400,
                "Invalid login data."
            );
        }

        // Ensures both required fields are present
        if (!body.has("login") ||
            !body.has("password")) {

            return createJsonErrorResponse(
                400,
                "Login and password are required."
            );
        }

        // Make sure both login fields contain string values
        if (body["login"].t() != crow::json::type::String ||
            body["password"].t() != crow::json::type::String) {

                return createJsonErrorResponse(
                    400,
                    "Login and password must be text values."
                );
            }
        
        // Read the submitted login value and password
        string login = body["login"].s();
        string password = body["password"].s();
        
        // Reject empty login fields
        if (isBlank(login) || isBlank(password)) {
            return createJsonErrorResponse(
                400,
                "Login and password cannot be empty."
            );
        }

        // Retrieve the stored password hash for the username or email
        int userId;
        string passwordHash;

        DatabaseResult loginResult = database.getUserLoginData(
            login,
            userId,
            passwordHash
        );

        if (loginResult == DatabaseResult::NotFound) {
            return createJsonErrorResponse(
                401,
                "Invalid login credentials."
            );
        }

        if (loginResult == DatabaseResult::Error) {
            return createJsonErrorResponse(
                500,
                "Unable to process login."
            );
        }

        // Verify the submitted password against the stored password hash
        if (!verifyPassword(password, passwordHash)) {
            return createJsonErrorResponse(
                401,
                "Invalid login credentials."
            );
        }

        // Generate a secure session token and expiration timestamp
        string sessionToken = generateSessionToken();
        string expiresAt = createSessionExpiration();

        // Store the new session in the database
        DatabaseResult sessionResult = database.createSession(
            userId,
            sessionToken,
            expiresAt
        );

        if (sessionResult == DatabaseResult::Error) {
            return createJsonErrorResponse(
                500,
                "Unable to create session."
            );
        }

        // Create the successful login response
        crow::response response = createJsonSuccessResponse(
                200,
                "Login successful."
            );

        // Store the session token in an HTTP-only cookie
        response.add_header(
            "Set-Cookie",
            "session_token=" + sessionToken +
            "; HttpOnly; SameSite=Strict; Path=/"
        );

        return response;
    
    });

    // Logs out the current user by deleting their session
    CROW_ROUTE(app, "/api/logout").methods(crow::HTTPMethod::POST)
    ([&database](const crow::request& request) {

        // Read the Cookie header from the request
        string cookieHeader = request.get_header_value("Cookie");

        // Extract the session token from the Cookie header
        string sessionToken;

        if (!getSessionTokenFromCookie(cookieHeader, sessionToken)) {
            return createJsonErrorResponse(
                401,
                "No active session found."
            );
        }

        // Delete the matching session from the database
        DatabaseResult logoutResult = database.deleteSession(sessionToken);

        if (logoutResult == DatabaseResult::NotFound) {
            return createJsonErrorResponse(
                401,
                "No active session found."
            );
        }

        if (logoutResult == DatabaseResult::Error) {
            return createJsonErrorResponse(
                500,
                "Unable to log out."
            );
        }

        // Create the successful logout response
        crow::response response = createJsonSuccessResponse(
            200,
            "Logout successful."
        );

        // Expire the browser's session cookie immediately
        response.add_header(
            "Set-Cookie",
            "session_token=; "
            "HttpOnly; "
            "SameSite=Strict; "
            "Path=/; "
            "Max-Age=0"
        );

        return response;

    });

    // Checks whether the current request has a valid authenticated session
    CROW_ROUTE(app, "/api/session").methods(crow::HTTPMethod::GET)
    ([&database](const crow::request& request) {

        int userId;
        string authenticationError;

        // Validate the session cookie using authentication function
        AuthenticationResult authenticationResult = authenticateRequest(
            request,
            database,
            userId,
            authenticationError
        );

        // The request does not contain a valid authenticated session
        if (authenticationResult == AuthenticationResult::Unauthorized) {
            return createJsonErrorResponse(
                401,
                authenticationError
            );
        }

        // An internal error prevented session validation
        if (authenticationResult == AuthenticationResult::Error) {
            return createJsonErrorResponse(
                500,
                authenticationError
            );
        }

        return createJsonSuccessResponse(
            200,
            "Authenticated."
        );
    });

    // Creates a completed combat-sports session for the authenticated user
    CROW_ROUTE(app, "/api/combat-sports/sessions")
    .methods(crow::HTTPMethod::POST)
    ([&database](const crow::request& request) {

        int userId;
        string authenticationError;

        // Authenticate the request and retrieve the current user's ID
        AuthenticationResult authenticationResult = authenticateRequest(
            request,
            database,
            userId,
            authenticationError
        );

        if (authenticationResult == AuthenticationResult::Unauthorized) {
            return createJsonErrorResponse(
                401,
                authenticationError
            );
        }

        if (authenticationResult == AuthenticationResult::Error) {
            return createJsonErrorResponse(
                500,
                authenticationError
            );
        }

        // Parse the submitted session information
        auto body = crow::json::load(request.body);

        if (!body) {
            return createJsonErrorResponse(
                400,
                "Invalid combat-sports session data."
            );
        }

        // Make sure every required field is present
        if (!body.has("discipline") ||
            !body.has("training_type") ||
            !body.has("session_date") ||
            !body.has("duration_minutes") ||
            !body.has("recording_method")) {

            return createJsonErrorResponse(
                400,
                "Discipline, training type, session date, duration, and recording method are required."
            );
        }

        // Required text fields must contain JSON string values
        if (body["discipline"].t() != crow::json::type::String ||
            body["training_type"].t() != crow::json::type::String ||
            body["session_date"].t() != crow::json::type::String ||
            body["recording_method"].t() != crow::json::type::String) {

            return createJsonErrorResponse(
                400,
                "Discipline, training type, session date, and recording method must be text values."
            );
        }

        // Duration must be provided as a JSON number
        if (body["duration_minutes"].t() != crow::json::type::Number) {
            return createJsonErrorResponse(
                400,
                "Duration must be a number of minutes."
            );
        }

        // Notes are optional, but they must be text when provided
        if (body.has("notes") &&
            body["notes"].t() != crow::json::type::String) {

            return createJsonErrorResponse(
                400,
                "Notes must be a text value."
            );
        }

        // Read the validated JSON fields
        string discipline = body["discipline"].s();
        string trainingType = body["training_type"].s();
        string sessionDate = body["session_date"].s();
        int durationMinutes = body["duration_minutes"].i();
        string recordingMethod = body["recording_method"].s();

        // Use an empty string when optional notes were not submitted
        string notes = "";

        if (body.has("notes")) {
            notes = body["notes"].s();
        }

        // Reject required text fields that contain no meaningful characters
        if (isBlank(discipline) ||
            isBlank(trainingType) ||
            isBlank(sessionDate) ||
            isBlank(recordingMethod)) {

            return createJsonErrorResponse(
                400,
                "Required session fields cannot be empty."
            );
        }

        if (durationMinutes <= 0) {
            return createJsonErrorResponse(
                400,
                "Duration must be greater than zero minutes."
            );
        }

        // These values match the recording_method constraint in the database
        if (recordingMethod != "manual" &&
            recordingMethod != "training_mode") {

            return createJsonErrorResponse(
                400,
                "Recording method must be manual or training_mode."
            );
        }

        int sessionId;

        DatabaseResult result = database.createCombatSportsSession(
            userId,
            discipline,
            trainingType,
            sessionDate,
            durationMinutes,
            recordingMethod,
            notes,
            sessionId
        );

        if (result == DatabaseResult::Conflict) {
            return createJsonErrorResponse(
                400,
                "The session contains unsupported values."
            );
        }

        if (result == DatabaseResult::Error) {
            return createJsonErrorResponse(
                500,
                "Unable to create combat-sports session."
            );
        }

        // Return the newly created session so the frontend can use it immediately
        crow::json::wvalue responseBody;

        responseBody["success"] = true;
        responseBody["message"] =
            "Combat-sports session created successfully.";

        responseBody["session"]["id"] = sessionId;
        responseBody["session"]["discipline"] = discipline;
        responseBody["session"]["training_type"] = trainingType;
        responseBody["session"]["session_date"] = sessionDate;
        responseBody["session"]["duration_minutes"] = durationMinutes;
        responseBody["session"]["recording_method"] = recordingMethod;
        responseBody["session"]["notes"] = notes;

        crow::response response(
            201,
            responseBody
        );

        response.add_header(
            "Content-Type",
            "application/json"
        );

        return response;
    });

    // Updates a combat-sports session belonging to the authenticated user
    CROW_ROUTE(app, "/api/combat-sports/sessions/<int>")
    .methods(crow::HTTPMethod::PUT)
    ([&database](
        const crow::request& request,
        int sessionId
    ) {
        int userId;
        string authenticationError;

        // Authenticate the request and retrieve the current user's ID
        AuthenticationResult authenticationResult = authenticateRequest(
            request,
            database,
            userId,
            authenticationError
        );

        if (authenticationResult == AuthenticationResult::Unauthorized) {
            return createJsonErrorResponse(
                401,
                authenticationError
            );
        }

        if (authenticationResult == AuthenticationResult::Error) {
            return createJsonErrorResponse(
                500,
                authenticationError
            );
        }

        // Route parameters should identify a valid database record
        if (sessionId <= 0) {
            return createJsonErrorResponse(
                400,
                "Session ID must be greater than zero."
            );
        }

        // Parse the complete set of updated session information
        auto body = crow::json::load(request.body);

        if (!body) {
            return createJsonErrorResponse(
                400,
                "Invalid combat-sports session data."
            );
        }

        // PUT replaces all editable values, so every required field
        // must be included in the request.
        if (!body.has("discipline") ||
            !body.has("training_type") ||
            !body.has("session_date") ||
            !body.has("duration_minutes") ||
            !body.has("recording_method")) {

            return createJsonErrorResponse(
                400,
                "Discipline, training type, session date, duration, and recording method are required."
            );
        }

        // Required text fields must contain JSON string values
        if (body["discipline"].t() != crow::json::type::String ||
            body["training_type"].t() != crow::json::type::String ||
            body["session_date"].t() != crow::json::type::String ||
            body["recording_method"].t() != crow::json::type::String) {

            return createJsonErrorResponse(
                400,
                "Discipline, training type, session date, and recording method must be text values."
            );
        }

        // Duration must be provided as a JSON number
        if (body["duration_minutes"].t() != crow::json::type::Number) {
            return createJsonErrorResponse(
                400,
                "Duration must be a number of minutes."
            );
        }

        // Notes are optional, but they must be text when provided
        if (body.has("notes") &&
            body["notes"].t() != crow::json::type::String) {

            return createJsonErrorResponse(
                400,
                "Notes must be a text value."
            );
        }

        // Read the validated JSON fields
        string discipline = body["discipline"].s();
        string trainingType = body["training_type"].s();
        string sessionDate = body["session_date"].s();
        int durationMinutes = body["duration_minutes"].i();
        string recordingMethod = body["recording_method"].s();

        // Use an empty string when optional notes were not submitted
        string notes = "";

        if (body.has("notes")) {
            notes = body["notes"].s();
        }

        // Reject required text fields containing no meaningful characters
        if (isBlank(discipline) ||
            isBlank(trainingType) ||
            isBlank(sessionDate) ||
            isBlank(recordingMethod)) {

            return createJsonErrorResponse(
                400,
                "Required session fields cannot be empty."
            );
        }

        if (durationMinutes <= 0) {
            return createJsonErrorResponse(
                400,
                "Duration must be greater than zero minutes."
            );
        }

        // These values match the recording_method database constraint
        if (recordingMethod != "manual" &&
            recordingMethod != "training_mode") {

            return createJsonErrorResponse(
                400,
                "Recording method must be manual or training_mode."
            );
        }

        // The database checks both IDs so users can update only
        // combat-sports sessions that belong to them.
        DatabaseResult result = database.updateCombatSportsSession(
            sessionId,
            userId,
            discipline,
            trainingType,
            sessionDate,
            durationMinutes,
            recordingMethod,
            notes
        );

        // A missing session and another user's session both return 404
        if (result == DatabaseResult::NotFound) {
            return createJsonErrorResponse(
                404,
                "Combat-sports session not found."
            );
        }

        if (result == DatabaseResult::Conflict) {
            return createJsonErrorResponse(
                400,
                "The session contains unsupported values."
            );
        }

        if (result == DatabaseResult::Error) {
            return createJsonErrorResponse(
                500,
                "Unable to update combat-sports session."
            );
        }

        // Return the updated values so the frontend can replace its
        // existing history entry without requesting the full history again.
        crow::json::wvalue responseBody;

        responseBody["success"] = true;
        responseBody["message"] =
            "Combat-sports session updated successfully.";

        responseBody["session"]["id"] = sessionId;
        responseBody["session"]["discipline"] = discipline;
        responseBody["session"]["training_type"] = trainingType;
        responseBody["session"]["session_date"] = sessionDate;
        responseBody["session"]["duration_minutes"] = durationMinutes;
        responseBody["session"]["recording_method"] = recordingMethod;
        responseBody["session"]["notes"] = notes;

        crow::response response(
            200,
            responseBody
        );

        response.add_header(
            "Content-Type",
            "application/json"
        );

        return response;
    });

    // Deletes a combat-sports session belonging to the authenticated user
    CROW_ROUTE(app, "/api/combat-sports/sessions/<int>")
    .methods(crow::HTTPMethod::Delete)
    ([&database](
        const crow::request& request,
        int sessionId
    ) {
        int userId;
        string authenticationError;

        // Authenticate the request and retrieve the current user's ID
        AuthenticationResult authenticationResult = authenticateRequest(
            request,
            database,
            userId,
            authenticationError
        );

        if (authenticationResult == AuthenticationResult::Unauthorized) {
            return createJsonErrorResponse(
                401,
                authenticationError
            );
        }

        if (authenticationResult == AuthenticationResult::Error) {
            return createJsonErrorResponse(
                500,
                authenticationError
            );
        }

        // Route parameters should identify a valid database record
        if (sessionId <= 0) {
            return createJsonErrorResponse(
                400,
                "Session ID must be greater than zero."
            );
        }

        // The database checks both IDs so users can delete only
        // combat-sports sessions that belong to them.
        DatabaseResult result = database.deleteCombatSportsSession(
            sessionId,
            userId
        );

        // A missing session and another user's session both return 404
        if (result == DatabaseResult::NotFound) {
            return createJsonErrorResponse(
                404,
                "Combat-sports session not found."
            );
        }

        if (result == DatabaseResult::Error) {
            return createJsonErrorResponse(
                500,
                "Unable to delete combat-sports session."
            );
        }

        // Return the deleted ID so the frontend can remove the
        // matching entry without requesting the full history again.
        crow::json::wvalue responseBody;

        responseBody["success"] = true;
        responseBody["message"] =
            "Combat-sports session deleted successfully.";
        responseBody["deleted_session_id"] = sessionId;

        crow::response response(
            200,
            responseBody
        );

        response.add_header(
            "Content-Type",
            "application/json"
        );

        return response;
    });

    // Retrieves all combat-sports sessions belonging to the authenticated user
    CROW_ROUTE(app, "/api/combat-sports/sessions")
    .methods(crow::HTTPMethod::GET)
    ([&database](const crow::request& request) {

        int userId;
        string authenticationError;

        // Authenticate the request and retrieve the current user's ID
        AuthenticationResult authenticationResult = authenticateRequest(
            request,
            database,
            userId,
            authenticationError
        );

        if (authenticationResult == AuthenticationResult::Unauthorized) {
            return createJsonErrorResponse(
                401,
                authenticationError
            );
        }

        if (authenticationResult == AuthenticationResult::Error) {
            return createJsonErrorResponse(
                500,
                authenticationError
            );
        }

        vector<CombatSportsSession> sessions;

        // Only retrieve sessions associated with the authenticated user
        DatabaseResult result = database.getCombatSportsSessions(
            userId,
            sessions
        );

        if (result == DatabaseResult::Error) {
            return createJsonErrorResponse(
                500,
                "Unable to retrieve combat-sports sessions."
            );
        }

        // Convert each database session into a JSON object
        crow::json::wvalue::list sessionList;

        for (const CombatSportsSession& session : sessions) {
            crow::json::wvalue sessionJson;

            sessionJson["id"] = session.id;
            sessionJson["discipline"] = session.discipline;
            sessionJson["training_type"] = session.trainingType;
            sessionJson["session_date"] = session.sessionDate;
            sessionJson["duration_minutes"] = session.durationMinutes;
            sessionJson["recording_method"] = session.recordingMethod;
            sessionJson["notes"] = session.notes;
            sessionJson["created_at"] = session.createdAt;
            sessionJson["updated_at"] = session.updatedAt;

            sessionList.push_back(
                std::move(sessionJson)
            );
        }

        // An empty session list is still a successful response
        crow::json::wvalue responseBody;

        responseBody["success"] = true;
        responseBody["sessions"] = std::move(sessionList);
        responseBody["count"] = static_cast<int>(sessions.size());

        crow::response response(
            200,
            responseBody
        );

        response.add_header(
            "Content-Type",
            "application/json"
        );

        return response;
    });
    
    // Health route used to verify that the web server is responding correctly
    CROW_ROUTE(app, "/api/health")([]() {
        return createJsonSuccessResponse(
            200,
            "Fitness Tracker API is healthy."
        );
    });

    // Handles request for routes that do not exist
    CROW_CATCHALL_ROUTE(app)
    ([]() {
        return createJsonErrorResponse(
            404,
            "404 - Page not found."
        );
    });
}
