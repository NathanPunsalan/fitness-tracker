// Reads the standardized JSON response returned by the Crow backend
async function handleApiResponse(response) {
    const data = await response.json();

    return {
        success: data.success,
        status: response.status,
        message: data.message || data.error,
        session: data.session,
        sessions: data.sessions,
        count: data.count,
        deletedSessionId: data.deleted_session_id
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
