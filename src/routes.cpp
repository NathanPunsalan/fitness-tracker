#include "routes.h"

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
void registerRoutes(crow::SimpleApp& app)
{
    // Home route used to confirm Fitness Tracker app is running
    CROW_ROUTE(app, "/")([]() {
        return crow::response(
            200,
            createPage(
                "Fitness Tracker",
                "Fitness Tracker is running. Issue 7"
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

    // Health route used to verify that the web server is responding correctly
    CROW_ROUTE(app, "/api/health")([]() {
        return crow::response(
            200,

            "Fitness Tracker API is healthy."
        );
    });
}