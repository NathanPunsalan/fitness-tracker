import { useState } from "react";
import { useNavigate } from "react-router-dom";

import Button from "../components/ui/Button";
import Card from "../components/ui/Card";
import FeedbackMessage from "../components/ui/FeedbackMessage";
import FormField from "../components/ui/FormField";
import { registerUser } from "../services/authService";

function Register() {
    // Store the values entered into each registration field
    const [username, setUsername] = useState("");
    const [email, setEmail] = useState("");
    const [password, setPassword] = useState("");

    // Store the text and visual type of the latest feedback message
    const [message, setMessage] = useState("");
    const [messageType, setMessageType] = useState("info");

    // Track whether a registration request is currently processing
    const [loading, setLoading] = useState(false);

    // Allows the application to redirect the user after registration
    const navigate = useNavigate();

    // Handles registration form submission
    async function handleSubmit(event) {
        // Prevent the browser from refreshing the page
        event.preventDefault();

        // Prevent duplicate submissions while registration is processing
        if (loading) {
            return;
        }

        // Clear any previous message and begin loading
        setMessage("");
        setLoading(true);

        try {
            // Send registration information to the backend
            const result = await registerUser(
                username,
                email,
                password
            );

            // Select the appropriate feedback style for the result
            setMessageType(
                result.success ? "success" : "error"
            );
            setMessage(result.message);

            // Redirect to the login page after successful registration
            if (result.success) {
                navigate("/login");
                return;
            }
        } catch (error) {
            // Display a safe error if the request unexpectedly fails
            setMessageType("error");
            setMessage(
                "Unable to create account. Please try again."
            );
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
            <h1>Create Account</h1>

            <form onSubmit={handleSubmit}>
                <FormField
                    id="username"
                    label="Username"
                    type="text"
                    value={username}
                    onChange={(event) =>
                        setUsername(event.target.value)
                    }
                    disabled={loading}
                    autoComplete="username"
                    required
                />

                <FormField
                    id="email"
                    label="Email"
                    type="email"
                    value={email}
                    onChange={(event) =>
                        setEmail(event.target.value)
                    }
                    disabled={loading}
                    autoComplete="email"
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
                    autoComplete="new-password"
                    required
                />

                <Button
                    type="submit"
                    loading={loading}
                    loadingText="Registering..."
                >
                    Register
                </Button>
            </form>

            <FeedbackMessage type={messageType}>
                {message}
            </FeedbackMessage>
        </Card>
    );
}

export default Register;
