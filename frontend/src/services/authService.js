// Reads a standardized JSON response from the Crow backend
async function handleApiResponse(response) {
    const data = await response.json();

    return {
        success: data.success,
        status: response.status,
        message: data.message || data.error
    };
}

// Sends a registration request to the Crow backend
export async function registerUser(username, email, password) {
    // Send the registration data as JSON
    const response = await fetch(
        "/api/register",
        {
            method: "POST",

            headers: {
                "Content-Type": "application/json"
            },

            body: JSON.stringify({
                username,
                email,
                password
            })
        }
    );

    return handleApiResponse(response);
}

// Sends a login request to the Crow backend
export async function loginUser(login, password) {
    // Send the login credentials as JSON
    const response = await fetch(
        "/api/login",
        {
            method: "POST",

            headers: {
                "Content-Type": "application/json"
            },

            body: JSON.stringify({
                login,
                password
            })
        }
    );

    return handleApiResponse(response);
}

// Sends a logout request to the Crow backend
export async function logoutUser() {
    // Ask the backend to delete the active session
    const response = await fetch(
        "/api/logout",
        {
            method: "POST"
        }
    );

    return handleApiResponse(response);
}

// Checks whether the current user has a valid authenticated session
export async function checkSession() {
    // Ask the backend to validate the current session cookie
    const response = await fetch(
        "/api/session",
        {
            method: "GET"
        }
    );

    return handleApiResponse(response);
}