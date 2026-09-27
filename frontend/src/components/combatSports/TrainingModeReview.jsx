import { useState } from "react";

import Button from "../ui/Button";
import FeedbackMessage from "../ui/FeedbackMessage";
import FormField from "../ui/FormField";
import SelectField from "../ui/SelectField";
import TextAreaField from "../ui/TextAreaField";

import "./TrainingModeReview.css";

function TrainingModeReview({ result, saving, error, onSave, onDiscard }) {
    const [activities, setActivities] = useState(result.results);
    const [notes, setNotes] = useState("");
    const [validationError, setValidationError] = useState("");

    function updateActivity(index, changes) {
        setActivities((items) => items.map((item, itemIndex) =>
            itemIndex === index ? { ...item, ...changes } : item
        ));
    }

    function changeStatus(index, status) {
        const item = activities[index];
        if (status === "skipped") {
            updateActivity(index, {
                status,
                completedValue: 0,
                completedSets: 0,
                actualDurationSeconds: null
            });
            return;
        }

        updateActivity(index, {
            status,
            completedValue: item.completedValue ||
                Number(item.activity.target_value),
            completedSets: item.completedSets ||
                Number(item.activity.target_sets),
            actualDurationSeconds:
                item.activity.target_type === "duration_seconds"
                    ? item.actualDurationSeconds ||
                        Number(item.activity.target_value) *
                        Number(item.activity.target_sets)
                    : null
        });
    }

    function validate() {
        for (const item of activities) {
            if (item.status === "skipped") {
                continue;
            }

            const hasCompletedTarget =
                Number(item.completedValue) > 0 &&
                Number(item.completedSets) > 0;
            const hasDuration = Number(item.actualDurationSeconds) > 0;

            if (!hasCompletedTarget && !hasDuration) {
                return `${item.activity.name} needs a completed target or duration.`;
            }
        }
        return "";
    }

    function handleSubmit(event) {
        event.preventDefault();
        const message = validate();
        setValidationError(message);
        if (!message) {
            onSave({ activities, notes: notes.trim() });
        }
    }

    return (
        <div className="training-review" role="dialog" aria-modal="true">
            <form className="training-review__panel" onSubmit={handleSubmit}>
                <header className="training-review__header">
                    <div>
                        <span className="training-review__eyebrow">
                            Workout Complete
                        </span>
                        <h1>Review {result.workout.name}</h1>
                        <p>
                            Confirm what you completed before saving the
                            session.
                        </p>
                    </div>
                    <div className="training-review__summary">
                        <strong>
                            {Math.ceil(result.actualDurationSeconds / 60)} min
                        </strong>
                        <span>
                            {result.stoppedEarly
                                ? "Stopped early"
                                : "Workout finished"}
                        </span>
                    </div>
                </header>

                <FeedbackMessage type="error">
                    {validationError || error}
                </FeedbackMessage>

                <div className="training-review__activities">
                    {activities.map((item, index) => (
                        <article
                            className="training-review__activity"
                            key={`${item.activity.id}-${index}`}
                        >
                            <div className="training-review__activity-heading">
                                <div>
                                    <span>{item.activity.roundName}</span>
                                    <h2>{item.activity.name}</h2>
                                </div>
                                <SelectField
                                    id={`result-status-${index}`}
                                    label="Status"
                                    value={item.status}
                                    onChange={(event) =>
                                        changeStatus(index, event.target.value)
                                    }
                                    disabled={saving}
                                >
                                    <option value="completed">Completed</option>
                                    <option value="partial">Partial</option>
                                    <option value="skipped">Skipped</option>
                                </SelectField>
                            </div>

                            {item.status !== "skipped" && (
                                <div className="training-review__values">
                                    <FormField
                                        id={`completed-value-${index}`}
                                        label="Completed Value"
                                        type="number"
                                        min="0"
                                        step="1"
                                        value={item.completedValue}
                                        onChange={(event) => updateActivity(
                                            index,
                                            { completedValue: Number(event.target.value) }
                                        )}
                                        disabled={saving}
                                    />
                                    <FormField
                                        id={`completed-sets-${index}`}
                                        label="Completed Sets"
                                        type="number"
                                        min="0"
                                        step="1"
                                        value={item.completedSets}
                                        onChange={(event) => updateActivity(
                                            index,
                                            { completedSets: Number(event.target.value) }
                                        )}
                                        disabled={saving}
                                    />
                                    <FormField
                                        id={`actual-duration-${index}`}
                                        label="Actual Seconds"
                                        type="number"
                                        min="1"
                                        step="1"
                                        value={item.actualDurationSeconds || ""}
                                        onChange={(event) => updateActivity(
                                            index,
                                            {
                                                actualDurationSeconds:
                                                    event.target.value
                                                        ? Number(event.target.value)
                                                        : null
                                            }
                                        )}
                                        disabled={saving}
                                    />
                                </div>
                            )}
                        </article>
                    ))}
                </div>

                <TextAreaField
                    id="training-result-notes"
                    label="Workout Notes (Optional)"
                    value={notes}
                    onChange={(event) => setNotes(event.target.value)}
                    disabled={saving}
                    rows="3"
                />

                <footer className="training-review__actions">
                    <Button
                        type="submit"
                        loading={saving}
                        loadingText="Saving workout..."
                    >
                        Save Workout
                    </Button>
                    <Button
                        type="button"
                        variant="secondary"
                        onClick={onDiscard}
                        disabled={saving}
                    >
                        Discard Result
                    </Button>
                </footer>
            </form>
        </div>
    );
}

export default TrainingModeReview;
