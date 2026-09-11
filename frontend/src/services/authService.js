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

    // Read the message returned by the backend
    const message = await response.text();

    // Return both the HTTP result and backend message
    return {
        success: response.ok,
        status: response.status,
        message
    };
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

    // Read the message returned by the backend
    const message = await response.text();

    // Return both the HTTP result and backend message
    return {
        success: response.ok,
        status: response.status,
        message
    };
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

    // Read the message returned by the backend
    const message = await response.text();

    // Return both the HTTP result and backend message
    return {
        success: response.ok,
        status: response.status,
        message
    };
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

    // Read the message returned by the backend
    const message = await response.text();

    // Return both the HTTP result and backend message
    return {
        success: response.ok,
        status: response.status,
        message
    };
}