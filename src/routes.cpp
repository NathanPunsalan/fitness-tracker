#include "routes.h"
#include "auth.h"

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
                "Fitness Tracker is running. Issue 8.3"
            )
        );
    });

    // Lifting placeholder route
    CROW_ROUTE(app, "/lifting")([]() {
        return crow::response(
            200,
            createPage(
                "Lifting",
                "Lifting features will be added in a future update."
            )
        );
    });

    // Running placeholder route
    CROW_ROUTE(app, "/running")([]() {
        return crow::response(
            200,
            createPage(
                "Running",
                "Running features will be added in a future update."
            )
        );
    });

    // Nutrition placeholder route
    CROW_ROUTE(app, "/nutrition")([]() {
        return crow::response(
            200,
            createPage(
                "Nutrition",
                "Nutrition features will be added in a future update."
            )
        );
    });

    // Account placeholder route
    CROW_ROUTE(app, "/account")([]() {
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
        
        // Read registration fields from the request
        string username = body["username"].s();
        string email = body["email"].s();
        string password = body["password"].s();

        // Reject empty registration fields
        if (username.empty() || email.empty() || password.empty() ) {
            return crow::response(
                400,
                "Username, email, and password cannot be empty."
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
        
        // Read the submitted login value and password
        string login = body["login"].s();
        string password = body["password"].s();
        
        // Reject empty login fields
        if (login.empty() || password.empty()) {
            return crow::response(
                400,
                "Login and password cannot be empty."
            );
        }

        // Retrieve the stored password hash for the username or email
        string passwordHash;

        if (!database.getUserPasswordHash(login, passwordHash)) {
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

        return crow::response(
            200,
            "Login successful."
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