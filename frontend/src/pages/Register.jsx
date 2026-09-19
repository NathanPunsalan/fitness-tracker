import { useState } from "react";

import { registerUser } from "../services/authService";
import { useNavigate } from "react-router-dom";

function Register() {
    // Store the values entered into each registration field
    const [username, setUsername] = useState("");
    const [email, setEmail] = useState("");
    const [password, setPassword] = useState("");

    // Store the message returned after registration
    const [message, setMessage] = useState("");

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
            // Send registration information to backend
            const result = await registerUser(
                username,
                email,
                password
            );

            // Display backend response to user
            setMessage(result.message);

            // Redirect to the login page after successful registration
            if (result.success) {
                navigate("/login");
                return;
            }
        } catch (error) {
            // Display a safe message if the request unexpectedly fails
            setMessage("Unable to create account. Please try again.");
        } finally {
            // End the loading state after the request finishes
            setLoading(false);
        }
    }

    return (
        <div>
            <h1>Create Account</h1>

            <form onSubmit={handleSubmit}>
                <div>
                    <label htmlFor="username">
                        Username
                    </label>

                    <input
                        id="username"
                        type="text"
                        value={username}
                        onChange={(event) =>
                            setUsername(event.target.value)
                        }
                        required
                    />
                </div>

                <div>
                    <label htmlFor="email">
                        Email
                    </label>

                    <input
                        id="email"
                        type="email"
                        value={email}
                        onChange={(event) =>
                            setEmail(event.target.value)
                        }
                        required
                    />
                </div>

                <div>
                    <label htmlFor="password">
                        Password
                    </label>

                    <input
                        id="password"
                        type="password"
                        value={password}
                        onChange={(event) =>
                            setPassword(event.target.value)
                        }
                        required
                    />
                </div>

                <button
                    type="submit"
                    disabled={loading}
                >
                    {loading ? "Registering..." : "Register"}
                </button>
            </form>

            {message && (
                <p>{message}</p>
            )}
        </div>
    );
}

export default Register;
