#include <crow.h>

int main()
{
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([]() {
        return "Fitness Tracker is running!";
    });

    app.port(18080).multithreaded().run();

    return 0;
}