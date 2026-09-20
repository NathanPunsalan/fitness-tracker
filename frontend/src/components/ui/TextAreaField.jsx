import "./FormField.css";

function TextAreaField({
    id,
    label,
    value,
    onChange,
    disabled = false,
    required = false,
    className = "",
    ...textAreaProps
}) {
    // Combine the shared field class with any additional
    // class supplied by the page using this component.
    const fieldClasses = [
        "ui-form-field",
        className
    ]
        .filter(Boolean)
        .join(" ");

    return (
        <div className={fieldClasses}>
            <label
                className="ui-form-field__label"
                htmlFor={id}
            >
                {label}
            </label>

            <textarea
                className={
                    "ui-form-field__input ui-form-field__textarea"
                }
                id={id}
                value={value}
                onChange={onChange}
                disabled={disabled}
                required={required}
                {...textAreaProps}
            />
        </div>
    );
}

export default TextAreaField;
