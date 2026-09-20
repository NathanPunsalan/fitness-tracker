// Reads the standardized JSON response returned by the Crow backend
async function handleApiResponse(response) {
    const data = await response.json();

    return {
        success: data.success,
        status: response.status,
        message: data.message || data.error,
        session: data.session
    };
}

// Sends a completed combat-sports session to the backend
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

            // Convert the React field names into the snake_case
            // property names expected by the Crow API.
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
