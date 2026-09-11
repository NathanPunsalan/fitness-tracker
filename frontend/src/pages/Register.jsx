import { useState } from "react";

import { registerUser } from "../services/authService";
import { useNavigate } from "react-router-dom";

function Register() {
    // Store the values entered into each registration field
    const [username, setUsername] = useState("");
    const [email, setEmail] = useState("");
    const [password, setPassword] = useState("");

    // Allows the application to redirect the user after registration
    const navigate = useNavigate();

    // Store the message returned after registration
    const [message, setMessage] = useState("");

    // Handles registration form submission
    async function handleSubmit(event) {
        // Prevent the browser from refreshing page
        event.preventDefault();

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

                <button type="submit">
                    Register
                </button>
            </form>

            {message && (
                <p>{message}</p>
            )}
        </div>
    );
}

export default Register;