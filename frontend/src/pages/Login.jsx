import { useState } from "react";
import { useNavigate } from "react-router-dom";

import Button from "../components/ui/Button";
import Card from "../components/ui/Card";
import FeedbackMessage from "../components/ui/FeedbackMessage";
import FormField from "../components/ui/FormField";
import { useAuth } from "../context/AuthContext";
import { loginUser } from "../services/authService";

function Login() {
    // Store the username/email and password entered by the user
    const [login, setLogin] = useState("");
    const [password, setPassword] = useState("");

    // Store the text and visual type of the latest feedback message
    const [message, setMessage] = useState("");
    const [messageType, setMessageType] = useState("info");

    // Track whether a login request is currently processing
    const [loading, setLoading] = useState(false);

    // Access the shared authentication state
    const { login: setAuthenticated } = useAuth();

    // Allows the application to redirect the user after login
    const navigate = useNavigate();

    // Handles login form submission
    async function handleSubmit(event) {
        // Prevent the browser from refreshing the page
        event.preventDefault();

        // Prevent duplicate submissions while login is processing
        if (loading) {
            return;
        }

        // Clear any previous message and begin loading
        setMessage("");
        setLoading(true);

        try {
            // Send login information to the backend
            const result = await loginUser(
                login,
                password
            );

            // Select the appropriate feedback style for the result
            setMessageType(
                result.success ? "success" : "error"
            );
            setMessage(result.message);

            // Update authentication state and redirect after successful login
            if (result.success) {
                setAuthenticated();
                navigate("/");
                return;
            }
        } catch (error) {
            // Display a safe error if the request unexpectedly fails
            setMessageType("error");
            setMessage("Unable to log in. Please try again.");
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
            <h1>Login</h1>

            <form onSubmit={handleSubmit}>
                <FormField
                    id="login"
                    label="Username or Email"
                    type="text"
                    value={login}
                    onChange={(event) =>
                        setLogin(event.target.value)
                    }
                    disabled={loading}
                    autoComplete="username"
                    required
                />

                <FormField
                    id="password"
                    label="Password"
                    type="password"
                    value={password}
                    onChange={(event) =>
                        setPassword(event.target.value)
                    }
                    disabled={loading}
                    autoComplete="current-password"
                    required
                />

                <Button
                    type="submit"
                    loading={loading}
                    loadingText="Logging in..."
                >
                    Login
                </Button>
            </form>

            <FeedbackMessage type={messageType}>
                {message}
            </FeedbackMessage>
        </Card>
    );
}

export default Login;
