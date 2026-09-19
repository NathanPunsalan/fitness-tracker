import { createContext, useContext, useEffect, useState } from "react";

import { checkSession } from "../services/authService";

// Create shared authentication context
const AuthContext = createContext(null);

export function AuthProvider({ children }) {
    // Track whether the initial session check is still running
    const [loading, setLoading] = useState(true);

    // Track whether the user is authenticated
    const [authenticated, setAuthenticated] = useState(false);

    // Check for an existing session when the application starts
    useEffect(() => {
        async function verifySession() {
            const result = await checkSession();

            setAuthenticated(result.success);
            setLoading(false);
        }

        verifySession();
    }, []);

    // Mark the user as authenticated after successful login
    function login() {
        setAuthenticated(true);
    }

    // Mark the user as unauthenticated after successful logout
    function logout() {
        setAuthenticated(false);
    }

    return (
        <AuthContext.Provider
            value={{
                authenticated,
                loading,
                login,
                logout
            }}
        >
            {children}
        </AuthContext.Provider>
    );
}

// Allow components to access the shared authentication state
export function useAuth() {
    return useContext(AuthContext);
}