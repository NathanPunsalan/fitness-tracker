#include <crow.h>
#include <iostream>

#include "database.h"

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

    // Basic route used to verify that the app is running
    CROW_ROUTE(app, "/")([]() {
        return "Fitness Tracker is running. Issue 4";
    });

    // Start the web server
    app.port(18080).multithreaded().run();

    return 0;
}