import { Navigate } from "react-router-dom";

import LoadingIndicator from "./ui/LoadingIndicator";
import { useAuth } from "../context/AuthContext";

function ProtectedRoute({ children }) {
    // Access the shared authentication state
    const { authenticated, loading } = useAuth();

    // Avoid rendering protected content until
    // the initial session check is complete.
    if (loading) {
        return (
            <div className="page-loading">
                <LoadingIndicator label="Checking authentication..." />
            </div>
        );
    }

    // Redirect unauthenticated users to the login page
    if (!authenticated) {
        return <Navigate to="/login" replace />;
    }

    // Render the protected page when authentication succeeds
    return children;
}

export default ProtectedRoute;
