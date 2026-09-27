import {
    useCallback,
    useEffect,
    useMemo,
    useRef,
    useState
} from "react";

import Button from "../ui/Button";

import "./CombatSportsTrainingMode.css";

function formatTime(totalSeconds) {
    const safeSeconds = Math.max(0, totalSeconds);
    const minutes = Math.floor(safeSeconds / 60);
    const seconds = safeSeconds % 60;
    return `${String(minutes).padStart(2, "0")}:${String(seconds).padStart(2, "0")}`;
}

function flattenWorkout(workout) {
    return workout.rounds.flatMap((round) =>
        round.activities.map((activity) => ({
            ...activity,
            roundId: round.id,
            roundName: round.name || `Round ${round.round_order}`,
            roundOrder: round.round_order
        }))
    );
}

function activityTarget(activity) {
    if (activity.target_type === "duration_seconds") {
        return Number(activity.target_value) * Number(activity.target_sets);
    }
    return Number(activity.target_value);
}

function targetLabel(activity) {
    const value = Number(activity.target_value);
    const sets = Number(activity.target_sets);

    if (activity.target_type === "repetitions") {
        return `${value} reps${sets > 1 ? ` × ${sets} sets` : ""}`;
    }
    if (activity.target_type === "rounds") {
        return `${value} rounds${sets > 1 ? ` × ${sets} sets` : ""}`;
    }
    return formatTime(value * sets);
}

function buildResult(activity, status, actualDurationSeconds = null) {
    const completed = status === "completed";
    return {
        activity,
        status,
        completedValue: completed ? Number(activity.target_value) : 0,
        completedSets: completed ? Number(activity.target_sets) : 0,
        actualDurationSeconds
    };
}

function CombatSportsTrainingMode({ workout, onExit, onReview }) {
    const activities = useMemo(() => flattenWorkout(workout), [workout]);
    const [currentIndex, setCurrentIndex] = useState(0);
    const [remainingSeconds, setRemainingSeconds] = useState(() =>
        activityTarget(activities[0])
    );
    const [elapsedSeconds, setElapsedSeconds] = useState(0);
    const [paused, setPaused] = useState(false);
    const advancingRef = useRef(false);
    const elapsedSecondsRef = useRef(0);
    const remainingSecondsRef = useRef(activityTarget(activities[0]));
    const resultsRef = useRef([]);
    const startedAtRef = useRef(new Date().toISOString());

    const current = activities[currentIndex];
    const next = activities[currentIndex + 1] || null;
    const timed = current?.target_type === "duration_seconds";
    const showNext = timed && remainingSeconds <= 10 && next;

    const finishWorkout = useCallback((finalResults, stoppedEarly) => {
        onReview({
            workout,
            results: finalResults,
            actualDurationSeconds: Math.max(1, elapsedSecondsRef.current),
            startedAt: startedAtRef.current,
            completedAt: new Date().toISOString(),
            stoppedEarly
        });
    }, [onReview, workout]);

    const advance = useCallback((status) => {
        if (advancingRef.current) {
            return;
        }
        advancingRef.current = true;

        const durationCompleted = timed
            ? Math.max(
                1,
                activityTarget(current) - remainingSecondsRef.current
            )
            : null;
        const result = buildResult(current, status, durationCompleted);
        const updatedResults = [...resultsRef.current, result];

        if (currentIndex === activities.length - 1) {
            finishWorkout(updatedResults, false);
            return;
        }

        const followingIndex = currentIndex + 1;
        const followingActivity = activities[followingIndex];
        resultsRef.current = updatedResults;
        setCurrentIndex(followingIndex);
        remainingSecondsRef.current = activityTarget(followingActivity);
        setRemainingSeconds(remainingSecondsRef.current);
        advancingRef.current = false;
    }, [activities, current, currentIndex, finishWorkout, timed]);

    function stopWorkout() {
        if (!window.confirm("Stop this workout early?")) {
            return;
        }

        const currentProgress = timed
            ? Math.max(0, activityTarget(current) - remainingSeconds)
            : 0;
        const currentStatus = currentProgress > 0 ? "partial" : "skipped";
        const finalResults = [
            ...resultsRef.current,
            buildResult(
                current,
                currentStatus,
                currentProgress > 0 ? currentProgress : null
            ),
            ...activities.slice(currentIndex + 1).map((activity) =>
                buildResult(activity, "skipped")
            )
        ];
        finishWorkout(finalResults, true);
    }

    useEffect(() => {
        if (paused || !current || !timed) {
            return undefined;
        }

        const timerId = window.setInterval(() => {
            elapsedSecondsRef.current += 1;
            setElapsedSeconds(elapsedSecondsRef.current);
            setRemainingSeconds((seconds) => {
                if (seconds <= 1) {
                    remainingSecondsRef.current = 0;
                    window.clearInterval(timerId);
                    window.setTimeout(() => advance("completed"), 0);
                    return 0;
                }
                remainingSecondsRef.current = seconds - 1;
                return remainingSecondsRef.current;
            });
        }, 1000);

        return () => window.clearInterval(timerId);
    }, [advance, current, paused, timed]);

    // Repetition and round targets have no countdown, but their time still
    // contributes to the actual workout duration while Training Mode runs.
    useEffect(() => {
        if (paused || !current || timed) {
            return undefined;
        }
        const timerId = window.setInterval(() => {
            elapsedSecondsRef.current += 1;
            setElapsedSeconds(elapsedSecondsRef.current);
        }, 1000);
        return () => window.clearInterval(timerId);
    }, [current, paused, timed]);

    if (!current) {
        return null;
    }

    return (
        <div className="training-mode" role="dialog" aria-modal="true">
            <header className="training-mode__header">
                <div>
                    <span className="training-mode__eyebrow">
                        {workout.name}
                    </span>
                    <h1>{current.roundName}</h1>
                </div>
                <div className="training-mode__progress">
                    Activity {currentIndex + 1} of {activities.length}
                    <span>{formatTime(elapsedSeconds)} elapsed</span>
                </div>
            </header>

            <main className="training-mode__main">
                <section className="training-mode__current">
                    <span className="training-mode__label">Current</span>
                    <h2>{current.name}</h2>
                    {current.instructions && <p>{current.instructions}</p>}

                    <div className="training-mode__timer" aria-live="polite">
                        {timed
                            ? formatTime(remainingSeconds)
                            : targetLabel(current)}
                    </div>

                    {!timed && Number(current.target_sets) > 1 && (
                        <span className="training-mode__target-note">
                            Complete every configured set before continuing.
                        </span>
                    )}
                </section>

                <section
                    className={`training-mode__next ${
                        showNext ? "training-mode__next--visible" : ""
                    }`}
                    aria-hidden={!showNext}
                >
                    <span className="training-mode__label">Next</span>
                    {showNext && (
                        <>
                            <h3>{next.name}</h3>
                            <p>{targetLabel(next)}</p>
                        </>
                    )}
                </section>
            </main>

            <footer className="training-mode__controls">
                <Button
                    type="button"
                    variant="secondary"
                    onClick={() => setPaused((value) => !value)}
                >
                    {paused ? "Resume" : "Pause"}
                </Button>
                {!timed && (
                    <Button type="button" onClick={() => advance("completed")}>
                        Complete Activity
                    </Button>
                )}
                <Button
                    type="button"
                    variant="secondary"
                    onClick={() => advance("skipped")}
                >
                    Skip
                </Button>
                <Button type="button" variant="danger" onClick={stopWorkout}>
                    Stop Workout
                </Button>
                <Button type="button" variant="secondary" onClick={onExit}>
                    Exit
                </Button>
            </footer>
        </div>
    );
}

export default CombatSportsTrainingMode;
