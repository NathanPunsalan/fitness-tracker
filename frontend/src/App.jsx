import { BrowserRouter, Routes, Route } from "react-router-dom";

import Dashboard from "./pages/Dashboard";
import Login from "./pages/Login";
import Register from "./pages/Register";
import Lifting from "./pages/Lifting";
import Running from "./pages/Running";
import Nutrition from "./pages/Nutrition";
import Account from "./pages/Account";

import "./App.css";

function App() {
    return (
        <BrowserRouter>
            <Routes>
                <Route path="/" element={<Dashboard />} />
                <Route path="/login" element={<Login />} />
                <Route path="/register" element={<Register />} />
                <Route path="/lifting" element={<Lifting />} />
                <Route path="/running" element={<Running />} />
                <Route path="/nutrition" element={<Nutrition />} />
                <Route path="/account" element={<Account />} />
            </Routes>
        </BrowserRouter>
    );
}

export default App;