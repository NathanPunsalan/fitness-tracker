#ifndef COMBAT_SPORTS_WORKOUT_ROUTES_H
#define COMBAT_SPORTS_WORKOUT_ROUTES_H

#include <crow.h>

#include "database.h"

// Registers authenticated workout-template CRUD and duplication routes.
void registerCombatSportsWorkoutRoutes(
    crow::SimpleApp& app,
    Database& database
);

#endif
