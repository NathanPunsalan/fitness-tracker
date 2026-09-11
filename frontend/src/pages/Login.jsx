import { useState } from "react";

import { loginUser } from "../services/authService";
import { useNavigate } from "react-router-dom";

function Login() {
    // Store the username/email and password entered by the user
    const [login, setLogin] = useState("");
    const [password, setPassword] = useState("");

    // Store the message returned after login
    const [message, setMessage] = useState("");

    // Allows the application to redirect the user after login
    const navigate = useNavigate();

    // Handles login form submission
    async function handleSubmit(event) {
        // Prevent the browser from refreshing the page
        event.preventDefault();

        // Send login information to the backend
        const result = await loginUser(
            login,
            password
        );

        // Display backend response to the user
        setMessage(result.message);

        if (result.success) {
            navigate("/");
        }
    }

    return (
        <div>
            <h1>Login</h1>
        

        <form onSubmit={handleSubmit}>
            <div>
                <label htmlFor="login">
                    Username or Email
                </label>

                <input
                    id="login"
                    type="text"
                    value={login}
                    onChange={(event) =>
                        setLogin(event.target.value)
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
                Login
            </button>
        </form>
        
        {message && (
            <p>{message}</p>
        )}
        </div>
    );

}

export default Login;