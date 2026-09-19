import LoadingIndicator from "./LoadingIndicator";
import "./Button.css";

function Button({
    children,
    variant = "primary",
    loading = false,
    loadingText = "Loading...",
    disabled = false,
    className = "",
    type = "button",
    ...buttonProps
}) {
    // Prevent interaction when the button is explicitly disabled
    // or while an asynchronous operation is processing.
    const isDisabled = disabled || loading;

    // Combine the base button class with the selected visual variant.
    const buttonClasses = [
        "ui-button",
        `ui-button--${variant}`,
        className
    ]
        .filter(Boolean)
        .join(" ");

    return (
        <button
            type={type}
            className={buttonClasses}
            disabled={isDisabled}
            aria-busy={loading}
            {...buttonProps}
        >
            {loading ? (
                <LoadingIndicator
                    label={loadingText}
                    size="small"
                />
            ) : (
                children
            )}
        </button>
    );
}

export default Button;
