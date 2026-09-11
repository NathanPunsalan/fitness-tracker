import { useEffect, useState } from "react";
import { Navigate } from "react-router-dom";

import { checkSession } from "../services/authService";

function ProtectedRoute({ children }) {
    // Track whether the session check is still running
    const [loading, setLoading] = useState(true);

    // Track whether the user has a valid authenticated session
    const [authenticated, setAuthenticated] = useState(false);

    useEffect(() => {
        async function verifySession() {
            // Ask the backend whether the current session is valid
            const result = await checkSession();

            // Store the authentication result
            setAuthenticated(result.success);

            // Mark the session check as complete
            setLoading(false);
        }

        verifySession();
    }, []);

    // Avoid rendering protected content until session validation finishes
    if (loading) {
        return <p>Checking authentication...</p>;
    }

    // Redirect unauthenticated users to the login page
    if (!authenticated) {
        return <Navigate to="/login" replace />;
    }

    // Render the protected page when authentication succeeds
    return children;
}

export default ProtectedRoute;