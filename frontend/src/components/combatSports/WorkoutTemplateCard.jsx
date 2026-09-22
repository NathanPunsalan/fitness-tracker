import { useState } from "react";

import Button from "../ui/Button";
import FeedbackMessage from "../ui/FeedbackMessage";
import FormField from "../ui/FormField";

function WorkoutTemplateCard({
    workout,
    busyAction,
    onEdit,
    onDuplicate,
    onDelete
}) {
    const [showDuplicate, setShowDuplicate] = useState(false);
    const [duplicateName, setDuplicateName] = useState(
        `${workout.name} Copy`
    );
    const [duplicateError, setDuplicateError] = useState("");

    const activityCount = workout.rounds.reduce(
        (total, round) => total + round.activities.length,
        0
    );
    const busy = busyAction?.workoutId === workout.id;

    async function handleDuplicate(event) {
        event.preventDefault();
        setDuplicateError("");

        const succeeded = await onDuplicate(
            workout.id,
            duplicateName.trim()
        );

        if (succeeded) {
            setShowDuplicate(false);
            setDuplicateName(`${workout.name} Copy`);
            return;
        }

        setDuplicateError(
            "Unable to create this copy. Choose a different workout name."
        );
    }

    return (
        <article className="workout-card">
            <div className="workout-card__header">
                <div>
                    <h3>{workout.name}</h3>
                    {workout.description && (
                        <p>{workout.description}</p>
                    )}
                </div>

                <div className="workout-card__summary">
                    <span>
                        {workout.rounds.length}{" "}
                        {workout.rounds.length === 1 ? "round" : "rounds"}
                    </span>
                    <span>
                        {activityCount}{" "}
                        {activityCount === 1 ? "activity" : "activities"}
                    </span>
                </div>
            </div>

            <div className="workout-card__disciplines">
                {workout.disciplines.map((discipline) => (
                    <span key={discipline}>{discipline}</span>
                ))}
            </div>

            <ol className="workout-card__rounds">
                {workout.rounds.map((round) => (
                    <li key={round.id}>
                        <strong>
                            {round.name || `Round ${round.round_order}`}
                        </strong>
                        <span>
                            {round.activities
                                .map((activity) => activity.name)
                                .join(" • ")}
                        </span>
                    </li>
                ))}
            </ol>

            <div className="workout-card__actions">
                <Button
                    type="button"
                    variant="secondary"
                    onClick={() => onEdit(workout)}
                    disabled={busy}
                >
                    Edit
                </Button>

                <Button
                    type="button"
                    variant="secondary"
                    onClick={() => {
                        setShowDuplicate((visible) => !visible);
                        setDuplicateError("");
                    }}
                    disabled={busy}
                >
                    Duplicate
                </Button>

                <Button
                    type="button"
                    variant="danger"
                    loading={busyAction?.type === "delete" && busy}
                    loadingText="Deleting..."
                    onClick={() => onDelete(workout)}
                    disabled={busy}
                >
                    Delete
                </Button>
            </div>

            {showDuplicate && (
                <form
                    className="workout-card__duplicate"
                    onSubmit={handleDuplicate}
                >
                    <FormField
                        id={`duplicate-workout-${workout.id}`}
                        label="New Workout Name"
                        value={duplicateName}
                        onChange={(event) =>
                            setDuplicateName(event.target.value)
                        }
                        disabled={busy}
                        required
                    />
                    <Button
                        type="submit"
                        loading={busyAction?.type === "duplicate" && busy}
                        loadingText="Duplicating..."
                        disabled={!duplicateName.trim() || busy}
                    >
                        Create Copy
                    </Button>

                    <FeedbackMessage type="error">
                        {duplicateError}
                    </FeedbackMessage>
                </form>
            )}
        </article>
    );
}

export default WorkoutTemplateCard;
