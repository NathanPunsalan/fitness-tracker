#ifndef ROUTES_H
#define ROUTES_H

#include <crow.h>
#include "database.h"

// Registers the application's web routes with the Crow Server
void registerRoutes(crow::SimpleApp& app, Database& database);

#endif