import "./Card.css";

function Card({
    children,
    shadow = false,
    className = "",
    as: Component = "div",
    ...cardProps
}) {
    // Apply the optional shadow style only when requested.
    const cardClasses = [
        "ui-card",
        shadow && "ui-card--shadow",
        className
    ]
        .filter(Boolean)
        .join(" ");

    // The "as" property allows the card to render as another
    // semantic element, such as a section or article.
    return (
        <Component
            className={cardClasses}
            {...cardProps}
        >
            {children}
        </Component>
    );
}

export default Card;
