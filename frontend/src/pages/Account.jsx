import { useState } from "react";
import { useNavigate } from "react-router-dom";

import { logoutUser } from "../services/authService";
import { useAuth } from "../context/AuthContext";

function Account() {
    // Store the message returned after logout
    const [message, setMessage] = useState("");

    // Access the shared authentication state
    const { logout } = useAuth();

    // Allows the application to redirect the user after logout
    const navigate = useNavigate();

    // Handles the logout request
    async function handleLogout() {
        // Ask the backend to delete the current session
        const result = await logoutUser();

        // Display the backend response to the user
        setMessage(result.message);

        // Update authentication state and redirect after successful logout
        if (result.success) {
            logout();
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