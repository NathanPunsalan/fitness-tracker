import { useRef, useState } from "react";

import Button from "../ui/Button";
import FeedbackMessage from "../ui/FeedbackMessage";
import FormField from "../ui/FormField";
import SelectField from "../ui/SelectField";
import TextAreaField from "../ui/TextAreaField";

const disciplineSuggestions = [
    "Muay Thai",
    "Brazilian Jiu-Jitsu",
    "Boxing",
    "Kickboxing",
    "MMA",
    "Wrestling",
    "Judo",
    "Conditioning"
];

const activityNames = {
    jump_rope: "Jump Rope",
    conditioning: "Conditioning",
    shadowboxing: "Shadowboxing",
    rest: "Rest",
    custom: "Custom Activity"
};

let nextEditorKey = 1;

function createActivity() {
    return {
        editorKey: nextEditorKey++,
        activityType: "shadowboxing",
        techniqueId: "",
        combinationId: "",
        drillId: "",
        name: "Shadowboxing",
        instructions: "",
        targetType: "duration_seconds",
        targetValue: "180",
        targetSets: "1",
        restAfterSeconds: "0"
    };
}

function createRound() {
    return {
        editorKey: nextEditorKey++,
        name: "",
        description: "",
        activities: [createActivity()]
    };
}

function createEmptyWorkout() {
    return {
        name: "",
        description: "",
        disciplines: [""],
        rounds: [createRound()]
    };
}

function hydrateWorkout(workout) {
    return {
        name: workout.name,
        description: workout.description || "",
        disciplines: [...workout.disciplines],
        rounds: workout.rounds.map((round) => ({
            editorKey: nextEditorKey++,
            name: round.name || "",
            description: round.description || "",
            activities: round.activities.map((activity) => ({
                editorKey: nextEditorKey++,
                activityType: activity.activity_type,
                techniqueId: activity.technique_id || "",
                combinationId: activity.combination_id || "",
                drillId: activity.drill_id || "",
                name: activity.name,
                instructions: activity.instructions || "",
                targetType: activity.target_type,
                targetValue: String(activity.target_value),
                targetSets: String(activity.target_sets),
                restAfterSeconds: String(activity.rest_after_seconds)
            }))
        }))
    };
}

function moveItem(items, index, direction) {
    const nextIndex = index + direction;
    if (nextIndex < 0 || nextIndex >= items.length) {
        return items;
    }

    const reordered = [...items];
    [reordered[index], reordered[nextIndex]] = [
        reordered[nextIndex],
        reordered[index]
    ];
    return reordered;
}

function WorkoutTemplateForm({
    editingWorkout,
    techniques,
    combinations,
    drills,
    loading,
    onSubmit,
    onCancel
}) {
    const [workout, setWorkout] = useState(() =>
        editingWorkout
            ? hydrateWorkout(editingWorkout)
            : createEmptyWorkout()
    );
    const [validationMessage, setValidationMessage] = useState("");
    const submissionInProgress = useRef(false);

    function setField(field, value) {
        setWorkout((current) => ({ ...current, [field]: value }));
    }

    function setDiscipline(index, value) {
        setWorkout((current) => ({
            ...current,
            disciplines: current.disciplines.map((discipline, itemIndex) =>
                itemIndex === index ? value : discipline
            )
        }));
    }

    function updateRound(roundIndex, changes) {
        setWorkout((current) => ({
            ...current,
            rounds: current.rounds.map((round, index) =>
                index === roundIndex ? { ...round, ...changes } : round
            )
        }));
    }

    function updateActivity(roundIndex, activityIndex, changes) {
        setWorkout((current) => ({
            ...current,
            rounds: current.rounds.map((round, index) =>
                index === roundIndex
                    ? {
                        ...round,
                        activities: round.activities.map(
                            (activity, itemIndex) =>
                                itemIndex === activityIndex
                                    ? { ...activity, ...changes }
                                    : activity
                        )
                    }
                    : round
            )
        }));
    }

    // Changing activity type clears incompatible references so a built-in
    // activity can never accidentally submit a previous library selection.
    function changeActivityType(roundIndex, activityIndex, activityType) {
        updateActivity(roundIndex, activityIndex, {
            activityType,
            techniqueId: "",
            combinationId: "",
            drillId: "",
            name: activityNames[activityType] || "",
            targetType:
                activityType === "technique" ||
                activityType === "combination"
                    ? "repetitions"
                    : "duration_seconds"
        });
    }

    function selectLibraryItem(
        roundIndex,
        activityIndex,
        activityType,
        selectedId
    ) {
        const source = activityType === "technique"
            ? techniques
            : activityType === "combination"
                ? combinations
                : drills;
        const selected = source.find(
            (item) => item.id === Number(selectedId)
        );

        updateActivity(roundIndex, activityIndex, {
            techniqueId: activityType === "technique" ? selectedId : "",
            combinationId:
                activityType === "combination" ? selectedId : "",
            drillId: activityType === "drill" ? selectedId : "",
            name: selected?.name || ""
        });
    }

    function validateWorkout() {
        if (!workout.name.trim()) {
            return "Please enter a workout name.";
        }

        const trimmedDisciplines = workout.disciplines.map(
            (discipline) => discipline.trim()
        );
        if (trimmedDisciplines.some((discipline) => !discipline)) {
            return "Every discipline must contain a value.";
        }
        if (new Set(trimmedDisciplines).size !== trimmedDisciplines.length) {
            return "Workout disciplines cannot contain duplicates.";
        }

        for (let roundIndex = 0; roundIndex < workout.rounds.length; roundIndex++) {
            const round = workout.rounds[roundIndex];
            if (round.activities.length === 0) {
                return `Round ${roundIndex + 1} must contain an activity.`;
            }

            for (
                let activityIndex = 0;
                activityIndex < round.activities.length;
                activityIndex++
            ) {
                const activity = round.activities[activityIndex];
                const location = `Round ${roundIndex + 1}, activity ${activityIndex + 1}`;
                if (!activity.name.trim()) {
                    return `${location} requires a name.`;
                }
                if (!Number.isInteger(Number(activity.targetValue)) ||
                    Number(activity.targetValue) <= 0) {
                    return `${location} requires a positive whole-number target.`;
                }
                if (!Number.isInteger(Number(activity.targetSets)) ||
                    Number(activity.targetSets) <= 0) {
                    return `${location} requires at least one set.`;
                }
                if (!Number.isInteger(Number(activity.restAfterSeconds)) ||
                    Number(activity.restAfterSeconds) < 0) {
                    return `${location} requires a non-negative rest value.`;
                }

                const reference = activity.activityType === "technique"
                    ? activity.techniqueId
                    : activity.activityType === "combination"
                        ? activity.combinationId
                        : activity.activityType === "drill"
                            ? activity.drillId
                            : null;
                if (reference === "") {
                    return `${location} requires a Training Library selection.`;
                }
            }
        }

        return "";
    }

    async function handleSubmit(event) {
        event.preventDefault();
        if (submissionInProgress.current) {
            return;
        }

        const error = validateWorkout();
        if (error) {
            setValidationMessage(error);
            return;
        }

        submissionInProgress.current = true;
        setValidationMessage("");

        const normalizedWorkout = {
            ...workout,
            name: workout.name.trim(),
            description: workout.description.trim(),
            disciplines: workout.disciplines.map((item) => item.trim()),
            rounds: workout.rounds.map((round) => ({
                ...round,
                name: round.name.trim(),
                description: round.description.trim(),
                activities: round.activities.map((activity) => ({
                    ...activity,
                    name: activity.name.trim(),
                    instructions: activity.instructions.trim()
                }))
            }))
        };

        try {
            const succeeded = await onSubmit(normalizedWorkout);
            if (succeeded && !editingWorkout) {
                setWorkout(createEmptyWorkout());
            }
        } finally {
            submissionInProgress.current = false;
        }
    }

    const selectedDisciplines = workout.disciplines
        .map((item) => item.trim())
        .filter(Boolean);
    const compatibleItems = (items) => items.filter(
        (item) => selectedDisciplines.includes(item.discipline)
    );

    return (
        <form className="workout-form" onSubmit={handleSubmit}>
            <h3>{editingWorkout ? "Edit Workout" : "Build a Workout"}</h3>

            <FormField
                id="workout-name"
                label="Workout Name"
                value={workout.name}
                onChange={(event) => setField("name", event.target.value)}
                disabled={loading}
                required
            />
            <TextAreaField
                id="workout-description"
                label="Description (Optional)"
                value={workout.description}
                onChange={(event) =>
                    setField("description", event.target.value)
                }
                disabled={loading}
                rows="3"
            />

            <fieldset className="workout-builder__group">
                <legend>Disciplines</legend>
                {workout.disciplines.map((discipline, index) => (
                    <div className="workout-builder__row" key={index}>
                        <FormField
                            id={`workout-discipline-${index}`}
                            label={`Discipline ${index + 1}`}
                            value={discipline}
                            onChange={(event) =>
                                setDiscipline(index, event.target.value)
                            }
                            list="workout-discipline-suggestions"
                            disabled={loading}
                            required
                        />
                        <Button
                            type="button"
                            variant="danger"
                            onClick={() => setWorkout((current) => ({
                                ...current,
                                disciplines: current.disciplines.filter(
                                    (_, itemIndex) => itemIndex !== index
                                )
                            }))}
                            disabled={loading || workout.disciplines.length === 1}
                        >
                            Remove
                        </Button>
                    </div>
                ))}
                <datalist id="workout-discipline-suggestions">
                    {disciplineSuggestions.map((item) => (
                        <option key={item} value={item} />
                    ))}
                </datalist>
                <Button
                    type="button"
                    variant="secondary"
                    onClick={() => setWorkout((current) => ({
                        ...current,
                        disciplines: [...current.disciplines, ""]
                    }))}
                    disabled={loading}
                >
                    Add Discipline
                </Button>
            </fieldset>

            <div className="workout-builder__rounds">
                {workout.rounds.map((round, roundIndex) => (
                    <fieldset
                        className="workout-round"
                        key={round.editorKey}
                    >
                        <legend>Round {roundIndex + 1}</legend>
                        <div className="workout-builder__toolbar">
                            <Button
                                type="button"
                                variant="secondary"
                                onClick={() => setWorkout((current) => ({
                                    ...current,
                                    rounds: moveItem(current.rounds, roundIndex, -1)
                                }))}
                                disabled={loading || roundIndex === 0}
                            >
                                Move Up
                            </Button>
                            <Button
                                type="button"
                                variant="secondary"
                                onClick={() => setWorkout((current) => ({
                                    ...current,
                                    rounds: moveItem(current.rounds, roundIndex, 1)
                                }))}
                                disabled={
                                    loading ||
                                    roundIndex === workout.rounds.length - 1
                                }
                            >
                                Move Down
                            </Button>
                            <Button
                                type="button"
                                variant="danger"
                                onClick={() => setWorkout((current) => ({
                                    ...current,
                                    rounds: current.rounds.filter(
                                        (_, index) => index !== roundIndex
                                    )
                                }))}
                                disabled={loading || workout.rounds.length === 1}
                            >
                                Remove Round
                            </Button>
                        </div>

                        <div className="workout-builder__two-column">
                            <FormField
                                id={`round-name-${round.editorKey}`}
                                label="Round Name (Optional)"
                                value={round.name}
                                onChange={(event) => updateRound(roundIndex, {
                                    name: event.target.value
                                })}
                                disabled={loading}
                            />
                            <FormField
                                id={`round-description-${round.editorKey}`}
                                label="Description (Optional)"
                                value={round.description}
                                onChange={(event) => updateRound(roundIndex, {
                                    description: event.target.value
                                })}
                                disabled={loading}
                            />
                        </div>

                        {round.activities.map((activity, activityIndex) => {
                            const libraryItems = activity.activityType === "technique"
                                ? compatibleItems(techniques)
                                : activity.activityType === "combination"
                                    ? compatibleItems(combinations)
                                    : compatibleItems(drills);
                            const selectedLibraryId =
                                activity.techniqueId ||
                                activity.combinationId ||
                                activity.drillId;
                            const isLibraryActivity = [
                                "technique", "combination", "drill"
                            ].includes(activity.activityType);

                            return (
                                <div
                                    className="workout-activity"
                                    key={activity.editorKey}
                                >
                                    <div className="workout-activity__header">
                                        <h4>Activity {activityIndex + 1}</h4>
                                        <div className="workout-builder__toolbar">
                                            <Button
                                                type="button"
                                                variant="secondary"
                                                onClick={() => updateRound(
                                                    roundIndex,
                                                    {
                                                        activities: moveItem(
                                                            round.activities,
                                                            activityIndex,
                                                            -1
                                                        )
                                                    }
                                                )}
                                                disabled={loading || activityIndex === 0}
                                            >
                                                Up
                                            </Button>
                                            <Button
                                                type="button"
                                                variant="secondary"
                                                onClick={() => updateRound(
                                                    roundIndex,
                                                    {
                                                        activities: moveItem(
                                                            round.activities,
                                                            activityIndex,
                                                            1
                                                        )
                                                    }
                                                )}
                                                disabled={
                                                    loading ||
                                                    activityIndex === round.activities.length - 1
                                                }
                                            >
                                                Down
                                            </Button>
                                            <Button
                                                type="button"
                                                variant="danger"
                                                onClick={() => updateRound(
                                                    roundIndex,
                                                    {
                                                        activities: round.activities.filter(
                                                            (_, index) => index !== activityIndex
                                                        )
                                                    }
                                                )}
                                                disabled={loading || round.activities.length === 1}
                                            >
                                                Remove
                                            </Button>
                                        </div>
                                    </div>

                                    <div className="workout-builder__two-column">
                                        <SelectField
                                            id={`activity-type-${activity.editorKey}`}
                                            label="Activity Type"
                                            value={activity.activityType}
                                            onChange={(event) => changeActivityType(
                                                roundIndex,
                                                activityIndex,
                                                event.target.value
                                            )}
                                            disabled={loading}
                                        >
                                            <option value="technique">Technique</option>
                                            <option value="combination">Combination</option>
                                            <option value="drill">Drill</option>
                                            <option value="jump_rope">Jump Rope</option>
                                            <option value="conditioning">Conditioning</option>
                                            <option value="shadowboxing">Shadowboxing</option>
                                            <option value="rest">Rest</option>
                                            <option value="custom">Custom</option>
                                        </SelectField>

                                        {isLibraryActivity ? (
                                            <SelectField
                                                id={`activity-reference-${activity.editorKey}`}
                                                label="Training Library Item"
                                                value={selectedLibraryId}
                                                onChange={(event) => selectLibraryItem(
                                                    roundIndex,
                                                    activityIndex,
                                                    activity.activityType,
                                                    event.target.value
                                                )}
                                                disabled={loading}
                                                required
                                            >
                                                <option value="">Select an item</option>
                                                {libraryItems.map((item) => (
                                                    <option key={item.id} value={item.id}>
                                                        {item.name} — {item.discipline}
                                                    </option>
                                                ))}
                                            </SelectField>
                                        ) : (
                                            <FormField
                                                id={`activity-name-${activity.editorKey}`}
                                                label="Activity Name"
                                                value={activity.name}
                                                onChange={(event) => updateActivity(
                                                    roundIndex,
                                                    activityIndex,
                                                    { name: event.target.value }
                                                )}
                                                disabled={loading}
                                                required
                                            />
                                        )}
                                    </div>

                                    <TextAreaField
                                        id={`activity-instructions-${activity.editorKey}`}
                                        label="Instructions (Optional)"
                                        value={activity.instructions}
                                        onChange={(event) => updateActivity(
                                            roundIndex,
                                            activityIndex,
                                            { instructions: event.target.value }
                                        )}
                                        disabled={loading}
                                        rows="2"
                                    />

                                    <div className="workout-builder__target-grid">
                                        <SelectField
                                            id={`target-type-${activity.editorKey}`}
                                            label="Target Type"
                                            value={activity.targetType}
                                            onChange={(event) => updateActivity(
                                                roundIndex,
                                                activityIndex,
                                                { targetType: event.target.value }
                                            )}
                                            disabled={loading}
                                        >
                                            <option value="repetitions">Repetitions</option>
                                            <option value="duration_seconds">Duration (Seconds)</option>
                                            <option value="rounds">Rounds</option>
                                        </SelectField>
                                        <FormField
                                            id={`target-value-${activity.editorKey}`}
                                            label="Target"
                                            type="number"
                                            min="1"
                                            step="1"
                                            value={activity.targetValue}
                                            onChange={(event) => updateActivity(
                                                roundIndex,
                                                activityIndex,
                                                { targetValue: event.target.value }
                                            )}
                                            disabled={loading}
                                            required
                                        />
                                        <FormField
                                            id={`target-sets-${activity.editorKey}`}
                                            label="Sets"
                                            type="number"
                                            min="1"
                                            step="1"
                                            value={activity.targetSets}
                                            onChange={(event) => updateActivity(
                                                roundIndex,
                                                activityIndex,
                                                { targetSets: event.target.value }
                                            )}
                                            disabled={loading}
                                            required
                                        />
                                        <FormField
                                            id={`rest-after-${activity.editorKey}`}
                                            label="Rest After (Seconds)"
                                            type="number"
                                            min="0"
                                            step="1"
                                            value={activity.restAfterSeconds}
                                            onChange={(event) => updateActivity(
                                                roundIndex,
                                                activityIndex,
                                                { restAfterSeconds: event.target.value }
                                            )}
                                            disabled={loading}
                                            required
                                        />
                                    </div>
                                </div>
                            );
                        })}

                        <Button
                            type="button"
                            variant="secondary"
                            onClick={() => updateRound(roundIndex, {
                                activities: [...round.activities, createActivity()]
                            })}
                            disabled={loading}
                        >
                            Add Activity
                        </Button>
                    </fieldset>
                ))}
            </div>

            <Button
                type="button"
                variant="secondary"
                onClick={() => setWorkout((current) => ({
                    ...current,
                    rounds: [...current.rounds, createRound()]
                }))}
                disabled={loading}
            >
                Add Round
            </Button>

            <FeedbackMessage type="error">
                {validationMessage}
            </FeedbackMessage>

            <div className="workout-form__actions">
                <Button
                    type="submit"
                    loading={loading}
                    loadingText={editingWorkout ? "Saving..." : "Creating..."}
                >
                    {editingWorkout ? "Save Workout" : "Create Workout"}
                </Button>
                {editingWorkout && (
                    <Button
                        type="button"
                        variant="secondary"
                        onClick={onCancel}
                        disabled={loading}
                    >
                        Cancel Edit
                    </Button>
                )}
            </div>
        </form>
    );
}

export default WorkoutTemplateForm;
