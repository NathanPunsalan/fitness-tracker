#include <crow.h>
#include <iostream>

#include "database.h"
#include "routes.h"

using namespace std;

int main()
{
    // Create the database object using the application's database file
    Database database("data/fitness_tracker.db");

    // Initialize the database before starting the server
    if (!database.initialize()) {
        cerr << "Unable to initialize the database." << endl;
        return 1;
    }

    // Create the Crow web application
    crow::SimpleApp app;

    // Register that application's routes with the Crow server
    registerRoutes(app);

    cout << "Starting Fitness Tracker server..." << endl;
    cout << "Open http://localhost:18080 in your browser." << endl;

    // Start the web server
    app.port(18080).multithreaded().run();

    return 0;
}