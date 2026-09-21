#ifndef COMBAT_SPORTS_CONTENT_ROUTES_H
#define COMBAT_SPORTS_CONTENT_ROUTES_H

#include <crow.h>

#include "database.h"

// Registers authenticated technique, combination, and drill API routes.
void registerCombatSportsContentRoutes(
    crow::SimpleApp& app,
    Database& database
);

#endif
