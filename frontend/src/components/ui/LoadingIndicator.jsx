import "./LoadingIndicator.css";

function LoadingIndicator({
    label = "Loading...",
    size = "medium",
    className = ""
}) {
    const loadingClasses = [
        "ui-loading",
        `ui-loading--${size}`,
        className
    ]
        .filter(Boolean)
        .join(" ");

    return (
        <span
            className={loadingClasses}
            role="status"
            aria-live="polite"
        >
            <span
                className="ui-loading__spinner"
                aria-hidden="true"
            />

            {label && (
                <span className="ui-loading__label">
                    {label}
                </span>
            )}
        </span>
    );
}

export default LoadingIndicator;
