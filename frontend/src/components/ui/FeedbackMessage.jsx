import "./FeedbackMessage.css";

function FeedbackMessage({
    children,
    type = "info",
    className = ""
}) {
    // Do not render an empty feedback container.
    if (!children) {
        return null;
    }

    const messageClasses = [
        "ui-feedback",
        `ui-feedback--${type}`,
        className
    ]
        .filter(Boolean)
        .join(" ");

    // Errors use an alert role because they require immediate attention.
    // Other messages use a status role for non-urgent updates.
    const role = type === "error" ? "alert" : "status";

    return (
        <div
            className={messageClasses}
            role={role}
            aria-live={type === "error" ? "assertive" : "polite"}
        >
            {children}
        </div>
    );
}

export default FeedbackMessage;
