#ifndef COMBAT_SPORTS_COMPLETED_WORKOUT_ROUTES_H
#define COMBAT_SPORTS_COMPLETED_WORKOUT_ROUTES_H

#include <crow.h>

#include "database.h"

// Registers authenticated routes for saving and retrieving completed workouts.
void registerCombatSportsCompletedWorkoutRoutes(
    crow::SimpleApp& app,
    Database& database
);

#endif
