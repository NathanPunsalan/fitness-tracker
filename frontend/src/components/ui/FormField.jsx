import "./FormField.css";

function FormField({
    id,
    label,
    type = "text",
    value,
    onChange,
    disabled = false,
    required = false,
    className = "",
    ...inputProps
}) {
    // Combine the standard field class with any additional class
    // supplied by the page using this component.
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

            <input
                className="ui-form-field__input"
                id={id}
                type={type}
                value={value}
                onChange={onChange}
                disabled={disabled}
                required={required}
                {...inputProps}
            />
        </div>
    );
}

export default FormField;
