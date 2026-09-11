#include "routes.h"
#include "auth.h"
#include "validation.h"

#include <string>

using namespace std;

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
                "Fitness Tracker is running. Issue 8.7"
            )
        );
    });

    // Lifting placeholder route
    CROW_ROUTE(app, "/lifting")
    ([&database](const crow::request& request) {
        
        int userId;
        string authenticationError;

        if(!authenticateRequest(
            request,
            database,
            userId,
            authenticationError
        )) {
            return crow::response(
                401,
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

        if (!authenticateRequest(
            request,
            database,
            userId,
            authenticationError
        )) {
            return crow::response(
            401,
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

        if (!authenticateRequest(
            request,
            database,
            userId,
            authenticationError
        )) {
            return crow::response(
            401,
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

        if (!authenticateRequest(
            request,
            database,
            userId,
            authenticationError
        )) {
            return crow::response(
            401,
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
            return crow::response(
                400,
                "Invalid registration data."
            );
        }

        // Make sure all required fields are present
        if (!body.has("username") ||
            !body.has("email") ||
            !body.has("password")) {
                
                return crow::response(
                    400,
                    "Username, email, and password are required."
                );
        }
        
        // Make sure all registration fields contain string values
        if (body["username"].t() != crow::json::type::String ||
            body["email"].t() != crow::json::type::String ||
            body["password"].t() != crow::json::type::String) {

                return crow::response(
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
            return crow::response(
                400,
                "Username, email, and password cannot be empty."
            );
        }

        // Validate username requirements
        if (!isValidUsername(username)) {
            return crow::response(
                400,
                "Username must be between 5 and 15 characters."
            );
        }

        // Validate email format
        if (!isValidEmail(email)) {
            return crow::response(
                400,
                "Please enter a valid email address."
            );
        }

        // Validate password security requirements
        if (!isValidPassword(password)) {
            return crow::response(
                400,
                "Password must be at least 8 characters and include an uppercase letter, lowercase letter, number, and symbol."
            );
        }

        // Hash the password before storing it in the database
        string passwordHash = hashPassword(password);

        // Attempt to create user account
        if (!database.createUser(username, email, passwordHash)) {
            return crow::response(
                400,
                "Unable to create user account."
            );
        }

        return crow::response(
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
            return crow::response(
                400,
                "Invalid login data."
            );
        }

        // Ensures both required fields are present
        if (!body.has("login") ||
            !body.has("password")) {

            return crow::response(
                400,
                "Login and password are required."
            );
        }

        // Make sure both login fields contain string values
        if (body["login"].t() != crow::json::type::String ||
            body["password"].t() != crow::json::type::String) {

                return crow::response(
                    400,
                    "Login and password must be text values."
                );
            }
        
        // Read the submitted login value and password
        string login = body["login"].s();
        string password = body["password"].s();
        
        // Reject empty login fields
        if (isBlank(login) || isBlank(password)) {
            return crow::response(
                400,
                "Login and password cannot be empty."
            );
        }

        // Retrieve the stored password hash for the username or email
        int userId;
        string passwordHash;

        if (!database.getUserLoginData(login, userId, passwordHash)) {
            return crow::response(
                401,
                "Invalid login credentials."
            );
        }

        // Verify the submitted password against the stored password hash
        if (!verifyPassword(password, passwordHash)) {
            return crow::response(
                401,
                "Invalid login credentials."
            );
        }

        // Generate a secure session token and expiration timestamp
        string sessionToken = generateSessionToken();
        string expiresAt = createSessionExpiration();

        // Store the new session in the database
        if (!database.createSession(userId, sessionToken, expiresAt)) {
            return crow::response(
                500,
                "Unable to create session."
            );
        }

        // Create the successful login response
        crow::response response(
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
            return crow::response(
                401,
                "No active session found."
            );
        }

        // Delete the matching session from the database
        if (!database.deleteSession(sessionToken)) {
            return crow::response(
                500,
                "Unable to log out."
            );
        }

        // Create the successful logout response
        crow::response response(
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
        if (!authenticateRequest(
            request,
            database,
            userId,
            authenticationError
        )) {
            return crow::response(
                401,
                authenticationError
            );
        }

        return crow::response(
            200,
            "Authenticated."
        );
    });
    
    // Health route used to verify that the web server is responding correctly
    CROW_ROUTE(app, "/api/health")([]() {
        return crow::response(
            200,
            "Fitness Tracker API is healthy."
        );
    });
}