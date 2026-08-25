#include "routes.h"

// Registers the application's basic routes
void registerRoutes(crow::SimpleApp& app)
{
    // Home route used to confirm Fitness Tracker app is running
    CROW_ROUTE(app, "/")([]() {
        return crow::response(
            200,
            "Fitness Tracker is running. Issue 6"
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