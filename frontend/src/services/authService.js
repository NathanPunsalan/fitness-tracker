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