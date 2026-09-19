import { NavLink } from "react-router-dom";

import { useAuth } from "../context/AuthContext";

function Navbar() {
    // Access the shared authentication state
    const { authenticated, loading } = useAuth();

    // Wait until the initial session check is complete
    if (loading) {
        return null;
    }

    return (
        <nav className="navbar">
            <NavLink
                to="/"
                className={({ isActive }) => isActive ? "active" : ""}
            >
                Dashboard
            </NavLink>

            {authenticated ? (
                <>
                    <NavLink
                        to="/lifting"
                        className={({ isActive }) => isActive ? "active" : ""}
                    >
                        Lifting
                    </NavLink>

                    <NavLink
                        to="/running"
                        className={({ isActive }) => isActive ? "active" : ""}
                    >
                        Running
                    </NavLink>

                    <NavLink
                        to="/nutrition"
                        className={({ isActive }) => isActive ? "active" : ""}
                    >
                        Nutrition
                    </NavLink>

                    <NavLink
                        to="/account"
                        className={({ isActive }) => isActive ? "active" : ""}
                    >
                        Account
                    </NavLink>
                </>
            ) : (
                <>
                    <NavLink
                        to="/login"
                        className={({ isActive }) => isActive ? "active" : ""}
                    >
                        Login
                    </NavLink>

                    <NavLink
                        to="/register"
                        className={({ isActive }) => isActive ? "active" : ""}
                    >
                        Register
                    </NavLink>
                </>
            )}
        </nav>
    );
}

export default Navbar;