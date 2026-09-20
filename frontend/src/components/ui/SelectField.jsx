import "./FormField.css";

function SelectField({
    id,
    label,
    value,
    onChange,
    children,
    disabled = false,
    required = false,
    className = "",
    ...selectProps
}) {
    // Combine the shared form-field class with any additional
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

            <select
                className="ui-form-field__input"
                id={id}
                value={value}
                onChange={onChange}
                disabled={disabled}
                required={required}
                {...selectProps}
            >
                {children}
            </select>
        </div>
    );
}

export default SelectField;
