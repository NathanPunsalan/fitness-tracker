import { useState } from "react";
import { useNavigate } from "react-router-dom";

import Button from "../components/ui/Button";
import Card from "../components/ui/Card";
import FeedbackMessage from "../components/ui/FeedbackMessage";
import { useAuth } from "../context/AuthContext";
import { logoutUser } from "../services/authService";

function Account() {
    // Store the text and visual type of the latest feedback message
    const [message, setMessage] = useState("");
    const [messageType, setMessageType] = useState("info");

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

            // Select the appropriate feedback style for the result
            setMessageType(
                result.success ? "success" : "error"
            );
            setMessage(result.message);

            // Update authentication state and redirect after successful logout
            if (result.success) {
                logout();
                navigate("/login");
                return;
            }
        } catch (error) {
            // Display a safe error if the request unexpectedly fails
            setMessageType("error");
            setMessage("Unable to log out. Please try again.");
        } finally {
            // End the loading state after the request finishes
            setLoading(false);
        }
    }

    return (
        <Card
            as="section"
            className="auth-card"
            shadow
        >
            <h1>Account</h1>

            <Button
                variant="secondary"
                onClick={handleLogout}
                loading={loading}
                loadingText="Logging out..."
            >
                Logout
            </Button>

            <FeedbackMessage type={messageType}>
                {message}
            </FeedbackMessage>
        </Card>
    );
}

export default Account;
