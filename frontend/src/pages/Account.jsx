import { useState } from "react";
import { useNavigate } from "react-router-dom";

import { logoutUser } from "../services/authService";
import { useAuth } from "../context/AuthContext";

function Account() {
    // Store the message returned after logout
    const [message, setMessage] = useState("");

    // Track whether the logout request is currently processing
    const [loading, setLoading] = useState(false);

    // Access the shared authentication state
    const { logout } = useAuth();

    // Allows the application to redirect the user after logout
    const navigate = useNavigate();

    // Handles the logout request
    async function handleLogout() {
        // Prevent duplicate logout requests
        if (loading) {
            return;
        }

        // Clear any previous message and begin loading
        setMessage("");
        setLoading(true);

        try {
            // Ask the backend to delete the current session
            const result = await logoutUser();

            // Display the backend response to the user
            setMessage(result.message);

            // Update authentication state and redirect after successful logout
            if (result.success) {
                logout();
                navigate("/login");
                return;
            }
        } catch (error) {
            // Display a safe message if the request unexpectedly fails
            setMessage("Unable to log out. Please try again.");
        } finally {
            // End the loading state after the request finishes
            setLoading(false);
        }
    }

    return (
        <div>
            <h1>Account</h1>

            <button
                onClick={handleLogout}
                disabled={loading}
            >
                {loading ? "Logging out..." : "Logout"}
            </button>

            {message && (
                <p>{message}</p>
            )}
        </div>
    );
}

export default Account;
