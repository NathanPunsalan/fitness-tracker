// Reads the standardized JSON response returned by the Crow backend
async function handleApiResponse(response) {
    const data = await response.json();

    return {
        success: data.success,
        status: response.status,
        message: data.message || data.error,
        session: data.session,
        sessions: data.sessions,
        technique: data.technique,
        techniques: data.techniques,
        combination: data.combination,
        combinations: data.combinations,
        drill: data.drill,
        drills: data.drills,
        count: data.count,
        deletedSessionId: data.deleted_session_id,
        deletedTechniqueId: data.deleted_technique_id,
        deletedCombinationId: data.deleted_combination_id,
        deletedDrillId: data.deleted_drill_id
    };
}

// Creates a completed combat-sports session
export async function createCombatSportsSession({
    discipline,
    trainingType,
    sessionDate,
    durationMinutes,
    recordingMethod,
    notes
}) {
    const response = await fetch(
        "/api/combat-sports/sessions",
        {
            method: "POST",

            headers: {
                "Content-Type": "application/json"
            },

            body: JSON.stringify({
                discipline,
                training_type: trainingType,
                session_date: sessionDate,
                duration_minutes: durationMinutes,
                recording_method: recordingMethod,
                notes
            })
        }
    );

    return handleApiResponse(response);
}

// Retrieves the authenticated user's combat-sports session history
export async function getCombatSportsSessions() {
    const response = await fetch(
        "/api/combat-sports/sessions",
        {
            method: "GET"
        }
    );

    return handleApiResponse(response);
}

// Updates a combat-sports session belonging to the authenticated user
export async function updateCombatSportsSession(
    sessionId,
    {
        discipline,
        trainingType,
        sessionDate,
        durationMinutes,
        recordingMethod,
        notes
    }
) {
    const response = await fetch(
        `/api/combat-sports/sessions/${sessionId}`,
        {
            method: "PUT",

            headers: {
                "Content-Type": "application/json"
            },

            // Convert React field names into the snake_case names
            // expected by the Crow update route.
            body: JSON.stringify({
                discipline,
                training_type: trainingType,
                session_date: sessionDate,
                duration_minutes: durationMinutes,
                recording_method: recordingMethod,
                notes
            })
        }
    );

    return handleApiResponse(response);
}

// Deletes a combat-sports session belonging to the authenticated user
export async function deleteCombatSportsSession(sessionId) {
    const response = await fetch(
        `/api/combat-sports/sessions/${sessionId}`,
        {
            method: "DELETE"
        }
    );

    return handleApiResponse(response);
}

// Sends a JSON request to one of the reusable combat-sports content routes.
async function sendContentRequest(path, method, body) {
    const options = { method };

    if (body !== undefined) {
        options.headers = {
            "Content-Type": "application/json"
        };
        options.body = JSON.stringify(body);
    }

    const response = await fetch(path, options);
    return handleApiResponse(response);
}

export function getCombatSportsTechniques() {
    return sendContentRequest("/api/combat-sports/techniques", "GET");
}

export function createCombatSportsTechnique(values) {
    return sendContentRequest(
        "/api/combat-sports/techniques",
        "POST",
        values
    );
}

export function updateCombatSportsTechnique(techniqueId, values) {
    return sendContentRequest(
        `/api/combat-sports/techniques/${techniqueId}`,
        "PUT",
        values
    );
}

export function deleteCombatSportsTechnique(techniqueId) {
    return sendContentRequest(
        `/api/combat-sports/techniques/${techniqueId}`,
        "DELETE"
    );
}

export function getCombatSportsCombinations() {
    return sendContentRequest("/api/combat-sports/combinations", "GET");
}

export function createCombatSportsCombination(values) {
    return sendContentRequest(
        "/api/combat-sports/combinations",
        "POST",
        values
    );
}

export function updateCombatSportsCombination(combinationId, values) {
    return sendContentRequest(
        `/api/combat-sports/combinations/${combinationId}`,
        "PUT",
        values
    );
}

export function deleteCombatSportsCombination(combinationId) {
    return sendContentRequest(
        `/api/combat-sports/combinations/${combinationId}`,
        "DELETE"
    );
}

export function getCombatSportsDrills() {
    return sendContentRequest("/api/combat-sports/drills", "GET");
}

export function createCombatSportsDrill(values) {
    return sendContentRequest(
        "/api/combat-sports/drills",
        "POST",
        values
    );
}

export function updateCombatSportsDrill(drillId, values) {
    return sendContentRequest(
        `/api/combat-sports/drills/${drillId}`,
        "PUT",
        values
    );
}

export function deleteCombatSportsDrill(drillId) {
    return sendContentRequest(
        `/api/combat-sports/drills/${drillId}`,
        "DELETE"
    );
}
