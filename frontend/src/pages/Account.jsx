import { useState } from "react";

import { logoutUser } from "../services/authService";
import { useNavigate } from "react-router-dom";

function Account() {
    // Store the message returned after logout
    const [message, setMessage] = useState("");

    // Allows the application to redirect the user after logout
    const navigate = useNavigate();

    // Handles the logout request
    async function handleLogout() {
        // Ask the backend to delete the current session
        const result = await logoutUser();

        // Display the backend response to the user
        setMessage(result.message);

        // Redirect to the login page after a successful logout
        if (result.success) {
            navigate("/login");
        }
    }

    return (
        <div>
            <h1>Account</h1>

            <button onClick={handleLogout}>
                Logout
            </button>

            {message && (
                <p>{message}</p>
            )}
        </div>
    );
}

export default Account;