import { useEffect, useRef, useState } from "react";

import Card from "../ui/Card";
import FeedbackMessage from "../ui/FeedbackMessage";
import LoadingIndicator from "../ui/LoadingIndicator";
import WorkoutTemplateCard from "./WorkoutTemplateCard";
import WorkoutTemplateForm from "./WorkoutTemplateForm";

import {
    createCombatSportsWorkout,
    deleteCombatSportsWorkout,
    duplicateCombatSportsWorkout,
    getCombatSportsCombinations,
    getCombatSportsDrills,
    getCombatSportsTechniques,
    getCombatSportsWorkouts,
    updateCombatSportsWorkout
} from "../../services/combatSportsService";

import "./CombatSportsWorkoutManager.css";

function replaceOrAdd(workouts, savedWorkout) {
    const exists = workouts.some((item) => item.id === savedWorkout.id);
    return exists
        ? workouts.map((item) =>
            item.id === savedWorkout.id ? savedWorkout : item
        )
        : [savedWorkout, ...workouts];
}

function CombatSportsWorkoutManager() {
    const [workouts, setWorkouts] = useState([]);
    const [techniques, setTechniques] = useState([]);
    const [combinations, setCombinations] = useState([]);
    const [drills, setDrills] = useState([]);
    const [editingWorkout, setEditingWorkout] = useState(null);
    const [loading, setLoading] = useState(true);
    const [saving, setSaving] = useState(false);
    const [busyAction, setBusyAction] = useState(null);
    const [message, setMessage] = useState("");
    const [messageType, setMessageType] = useState("info");
    const actionInProgress = useRef(false);

    useEffect(() => {
        let cancelled = false;

        async function refreshTrainingLibrary() {
            try {
                const results = await Promise.all([
                    getCombatSportsTechniques(),
                    getCombatSportsCombinations(),
                    getCombatSportsDrills()
                ]);

                if (cancelled || results.some((result) => !result.success)) {
                    return;
                }

                setTechniques(results[0].techniques || []);
                setCombinations(results[1].combinations || []);
                setDrills(results[2].drills || []);
            } catch {
                // The main feedback remains unchanged; a later library edit
                // or page reload can safely retry this background refresh.
            }
        }

        async function loadBuilderData() {
            try {
                const results = await Promise.all([
                    getCombatSportsWorkouts(),
                    getCombatSportsTechniques(),
                    getCombatSportsCombinations(),
                    getCombatSportsDrills()
                ]);

                if (cancelled) {
                    return;
                }

                const failed = results.find((result) => !result.success);
                if (failed) {
                    setMessageType("error");
                    setMessage(
                        failed.message || "Unable to load Workout Builder."
                    );
                    return;
                }

                setWorkouts(results[0].workouts || []);
                setTechniques(results[1].techniques || []);
                setCombinations(results[2].combinations || []);
                setDrills(results[3].drills || []);
            } catch {
                if (!cancelled) {
                    setMessageType("error");
                    setMessage(
                        "Unable to load Workout Builder. Please try again."
                    );
                }
            } finally {
                if (!cancelled) {
                    setLoading(false);
                }
            }
        }

        loadBuilderData();
        window.addEventListener(
            "combat-sports-library-changed",
            refreshTrainingLibrary
        );

        return () => {
            cancelled = true;
            window.removeEventListener(
                "combat-sports-library-changed",
                refreshTrainingLibrary
            );
        };
    }, []);

    async function saveWorkout(values) {
        if (actionInProgress.current) {
            return false;
        }

        actionInProgress.current = true;
        setSaving(true);
        setMessage("");

        try {
            const result = editingWorkout
                ? await updateCombatSportsWorkout(editingWorkout.id, values)
                : await createCombatSportsWorkout(values);

            setMessageType(result.success ? "success" : "error");
            if (!result.success || !result.workout) {
                setMessage(
                    result.message || "Unable to save the workout."
                );
                return false;
            }

            setWorkouts((items) => replaceOrAdd(items, result.workout));
            setMessage(
                editingWorkout
                    ? `${result.workout.name} updated successfully.`
                    : `${result.workout.name} created successfully.`
            );
            setEditingWorkout(null);
            return true;
        } catch {
            setMessageType("error");
            setMessage("Unable to save the workout. Please try again.");
            return false;
        } finally {
            actionInProgress.current = false;
            setSaving(false);
        }
    }

    async function duplicateWorkout(workoutId, name) {
        if (!name) {
            setMessageType("error");
            setMessage("Please enter a name for the duplicated workout.");
            return false;
        }
        if (actionInProgress.current) {
            return false;
        }

        actionInProgress.current = true;
        setBusyAction({ type: "duplicate", workoutId });
        setMessage("");

        try {
            const result = await duplicateCombatSportsWorkout(
                workoutId,
                name
            );
            setMessageType(result.success ? "success" : "error");

            if (!result.success || !result.workout) {
                setMessage(result.message || "Unable to duplicate workout.");
                return false;
            }

            setWorkouts((items) => [result.workout, ...items]);
            setMessage(`${result.workout.name} created successfully.`);
            return true;
        } catch {
            setMessageType("error");
            setMessage("Unable to duplicate workout. Please try again.");
            return false;
        } finally {
            actionInProgress.current = false;
            setBusyAction(null);
        }
    }

    async function deleteWorkout(workout) {
        if (!window.confirm(`Delete ${workout.name}?`)) {
            return;
        }
        if (actionInProgress.current) {
            return;
        }

        actionInProgress.current = true;
        setBusyAction({ type: "delete", workoutId: workout.id });
        setMessage("");

        try {
            const result = await deleteCombatSportsWorkout(workout.id);
            setMessageType(result.success ? "success" : "error");

            if (!result.success) {
                setMessage(result.message || "Unable to delete workout.");
                return;
            }

            setWorkouts((items) =>
                items.filter((item) => item.id !== workout.id)
            );
            if (editingWorkout?.id === workout.id) {
                setEditingWorkout(null);
            }
            setMessage(`${workout.name} deleted successfully.`);
        } catch {
            setMessageType("error");
            setMessage("Unable to delete workout. Please try again.");
        } finally {
            actionInProgress.current = false;
            setBusyAction(null);
        }
    }

    return (
        <Card as="section" className="workout-manager" shadow>
            <div className="workout-manager__header">
                <div>
                    <h2>Workout Builder</h2>
                    <p className="form-description">
                        Create ordered rounds for manual recording and future
                        Training Mode sessions.
                    </p>
                </div>
                <span className="workout-manager__count">
                    {workouts.length} workouts
                </span>
            </div>

            {loading ? (
                <div className="workout-manager__loading">
                    <LoadingIndicator label="Loading Workout Builder..." />
                </div>
            ) : (
                <>
                    <FeedbackMessage type={messageType}>
                        {message}
                    </FeedbackMessage>

                    <div className="workout-manager__grid">
                        <WorkoutTemplateForm
                            key={editingWorkout?.id || "new-workout"}
                            editingWorkout={editingWorkout}
                            techniques={techniques}
                            combinations={combinations}
                            drills={drills}
                            loading={saving}
                            onSubmit={saveWorkout}
                            onCancel={() => setEditingWorkout(null)}
                        />

                        <div className="workout-list">
                            <h3>Saved Workouts</h3>
                            {workouts.length === 0 ? (
                                <p className="workout-list__empty">
                                    No workouts yet. Build your first reusable
                                    routine using the form.
                                </p>
                            ) : (
                                workouts.map((workout) => (
                                    <WorkoutTemplateCard
                                        key={workout.id}
                                        workout={workout}
                                        busyAction={busyAction}
                                        onEdit={(selected) => {
                                            setEditingWorkout(selected);
                                            setMessage("");
                                        }}
                                        onDuplicate={duplicateWorkout}
                                        onDelete={deleteWorkout}
                                    />
                                ))
                            )}
                        </div>
                    </div>
                </>
            )}
        </Card>
    );
}

export default CombatSportsWorkoutManager;
