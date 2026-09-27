import { useEffect, useRef, useState } from "react";

import Card from "../ui/Card";
import FeedbackMessage from "../ui/FeedbackMessage";
import LoadingIndicator from "../ui/LoadingIndicator";
import WorkoutTemplateCard from "./WorkoutTemplateCard";
import WorkoutTemplateForm from "./WorkoutTemplateForm";
import CombatSportsTrainingMode from "./CombatSportsTrainingMode";
import TrainingModeReview from "./TrainingModeReview";

import {
    createCombatSportsWorkout,
    createCombatSportsCompletedWorkout,
    createCombatSportsSession,
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

function CombatSportsWorkoutManager({ onSessionCreated }) {
    const [workouts, setWorkouts] = useState([]);
    const [techniques, setTechniques] = useState([]);
    const [combinations, setCombinations] = useState([]);
    const [drills, setDrills] = useState([]);
    const [editingWorkout, setEditingWorkout] = useState(null);
    const [trainingWorkout, setTrainingWorkout] = useState(null);
    const [trainingResult, setTrainingResult] = useState(null);
    const [resultSaving, setResultSaving] = useState(false);
    const [resultError, setResultError] = useState("");
    const [loading, setLoading] = useState(true);
    const [saving, setSaving] = useState(false);
    const [busyAction, setBusyAction] = useState(null);
    const [message, setMessage] = useState("");
    const [messageType, setMessageType] = useState("info");
    const actionInProgress = useRef(false);
    const pendingSessionRef = useRef(null);

    function reviewTrainingResult(result) {
        setTrainingWorkout(null);
        setTrainingResult(result);
        setResultError("");
        pendingSessionRef.current = null;
    }

    async function saveTrainingResult(review) {
        if (actionInProgress.current || !trainingResult) {
            return;
        }

        actionInProgress.current = true;
        setResultSaving(true);
        setResultError("");

        try {
            let linkedSession = pendingSessionRef.current;
            if (!linkedSession) {
                const discipline = trainingResult.workout.disciplines[0];
                if (!discipline) {
                    setResultError("The workout needs at least one discipline.");
                    return;
                }

                const sessionResult = await createCombatSportsSession({
                    discipline,
                    trainingType: trainingResult.workout.name,
                    sessionDate: new Date().toLocaleDateString("en-CA"),
                    durationMinutes: Math.max(
                        1,
                        Math.ceil(trainingResult.actualDurationSeconds / 60)
                    ),
                    recordingMethod: "training_mode",
                    notes: review.notes
                });

                if (!sessionResult.success || !sessionResult.session) {
                    setResultError(
                        sessionResult.message || "Unable to create the session."
                    );
                    return;
                }
                linkedSession = sessionResult.session;
                pendingSessionRef.current = linkedSession;
            }

            const rounds = trainingResult.workout.rounds.map((round) => ({
                source_workout_round_id: round.id,
                name: round.name || `Round ${round.round_order}`,
                description: round.description || "",
                activities: review.activities
                    .filter((item) => item.activity.roundId === round.id)
                    .map((item) => ({
                        source_workout_activity_id: item.activity.id,
                        activity_type: item.activity.activity_type,
                        technique_id: item.activity.technique_id,
                        combination_id: item.activity.combination_id,
                        drill_id: item.activity.drill_id,
                        name: item.activity.name,
                        instructions: item.activity.instructions || "",
                        target_type: item.activity.target_type,
                        planned_value: Number(item.activity.target_value),
                        planned_sets: Number(item.activity.target_sets),
                        completed_value: Number(item.completedValue),
                        completed_sets: Number(item.completedSets),
                        actual_duration_seconds:
                            item.actualDurationSeconds || null,
                        status: item.status,
                        was_unplanned: false,
                        notes: ""
                    }))
            }));

            const completedResult =
                await createCombatSportsCompletedWorkout({
                    workout_template_id: trainingResult.workout.id,
                    combat_sports_session_id: linkedSession.id,
                    workout_name: trainingResult.workout.name,
                    workout_description:
                        trainingResult.workout.description || "",
                    recording_method: "training_mode",
                    started_at: trainingResult.startedAt,
                    completed_at: trainingResult.completedAt,
                    actual_duration_seconds:
                        trainingResult.actualDurationSeconds,
                    stopped_early: trainingResult.stoppedEarly,
                    notes: review.notes,
                    rounds
                });

            if (!completedResult.success) {
                setResultError(
                    completedResult.message || "Unable to save workout result."
                );
                return;
            }

            onSessionCreated?.(linkedSession);
            setTrainingResult(null);
            pendingSessionRef.current = null;
            setMessageType("success");
            setMessage(`${trainingResult.workout.name} saved successfully.`);
        } catch {
            setResultError("Unable to save this workout. Please try again.");
        } finally {
            actionInProgress.current = false;
            setResultSaving(false);
        }
    }

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
                                        onStartTraining={setTrainingWorkout}
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

            {trainingWorkout && (
                <CombatSportsTrainingMode
                    workout={trainingWorkout}
                    onExit={() => setTrainingWorkout(null)}
                    onReview={reviewTrainingResult}
                />
            )}

            {trainingResult && (
                <TrainingModeReview
                    result={trainingResult}
                    saving={resultSaving}
                    error={resultError}
                    onSave={saveTrainingResult}
                    onDiscard={() => {
                        setTrainingResult(null);
                        setResultError("");
                        pendingSessionRef.current = null;
                    }}
                />
            )}
        </Card>
    );
}

export default CombatSportsWorkoutManager;
