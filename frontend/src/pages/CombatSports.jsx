import {
    useEffect,
    useRef,
    useState
} from "react";

import CombatSportsSessionHistory from "../components/combatSports/CombatSportsSessionHistory";
import CombatSportsContentManager from "../components/combatSports/CombatSportsContentManager";
import Button from "../components/ui/Button";
import Card from "../components/ui/Card";
import FeedbackMessage from "../components/ui/FeedbackMessage";
import FormField from "../components/ui/FormField";
import SelectField from "../components/ui/SelectField";
import TextAreaField from "../components/ui/TextAreaField";

import {
    createCombatSportsSession,
    getCombatSportsSessions,
    updateCombatSportsSession
} from "../services/combatSportsService";

const disciplineSuggestions = [
    "Muay Thai",
    "Brazilian Jiu-Jitsu",
    "Boxing",
    "Kickboxing",
    "MMA",
    "Wrestling",
    "Judo"
];

const trainingTypeSuggestions = [
    "Class",
    "Open Mat",
    "Sparring",
    "Drilling",
    "Pad Work",
    "Bag Work",
    "Private Lesson",
    "Competition",
    "Conditioning"
];

// Produce today's date in the user's local timezone.
function getTodayDate() {
    const today = new Date();
    const year = today.getFullYear();
    const month = String(today.getMonth() + 1).padStart(2, "0");
    const day = String(today.getDate()).padStart(2, "0");

    return `${year}-${month}-${day}`;
}

// Keep sessions in the same newest-first order used by the backend.
function sortSessionsNewestFirst(sessions) {
    return [...sessions].sort((firstSession, secondSession) => {
        const dateComparison =
            secondSession.session_date.localeCompare(
                firstSession.session_date
            );

        if (dateComparison !== 0) {
            return dateComparison;
        }

        return secondSession.id - firstSession.id;
    });
}

function CombatSports() {
    const today = getTodayDate();

    // Store the current value of each form field.
    const [discipline, setDiscipline] = useState("");
    const [trainingType, setTrainingType] = useState("");
    const [sessionDate, setSessionDate] = useState(today);
    const [durationMinutes, setDurationMinutes] = useState("");
    const [recordingMethod, setRecordingMethod] =
        useState("manual");
    const [notes, setNotes] = useState("");

    // A null ID means the form is creating a new session.
    // A numeric ID means the form is editing that existing session.
    const [editingSessionId, setEditingSessionId] =
        useState(null);

    // Store the latest form feedback.
    const [message, setMessage] = useState("");
    const [messageType, setMessageType] = useState("info");

    // Track whether a create or update request is processing.
    const [loading, setLoading] = useState(false);

    // Store session-history state separately from form state.
    const [sessions, setSessions] = useState([]);
    const [historyLoading, setHistoryLoading] = useState(true);
    const [historyError, setHistoryError] = useState("");

    // This ref changes immediately, preventing rapid duplicate submissions.
    const submissionInProgress = useRef(false);

    // Load the authenticated user's session history when the page opens.
    useEffect(() => {
        let requestCancelled = false;

        async function loadSessionHistory() {
            try {
                const result = await getCombatSportsSessions();

                if (requestCancelled) {
                    return;
                }

                if (result.success) {
                    setSessions(
                        Array.isArray(result.sessions)
                            ? result.sessions
                            : []
                    );
                    return;
                }

                setHistoryError(
                    result.message ||
                    "Unable to load session history."
                );
            } catch {
                if (!requestCancelled) {
                    setHistoryError(
                        "Unable to load session history. Please try again."
                    );
                }
            } finally {
                if (!requestCancelled) {
                    setHistoryLoading(false);
                }
            }
        }

        loadSessionHistory();

        // Prevent state updates if the component unmounts before
        // the asynchronous request finishes.
        return () => {
            requestCancelled = true;
        };
    }, []);

    // Restore the form to its default create-session state.
    function resetForm() {
        setDiscipline("");
        setTrainingType("");
        setSessionDate(today);
        setDurationMinutes("");
        setRecordingMethod("manual");
        setNotes("");
        setEditingSessionId(null);
    }

    // Populate the form with an existing session and enter edit mode.
    function beginEditingSession(session) {
        setDiscipline(session.discipline);
        setTrainingType(session.training_type);
        setSessionDate(session.session_date);
        setDurationMinutes(
            String(session.duration_minutes)
        );
        setRecordingMethod(session.recording_method);
        setNotes(session.notes || "");
        setEditingSessionId(session.id);

        // Clear feedback from an earlier create or update operation.
        setMessage("");
        setMessageType("info");
    }

    // Leave edit mode without changing the selected database record.
    function cancelEditingSession() {
        resetForm();
        setMessage("");
        setMessageType("info");
    }

    // Remove a successfully deleted session from the displayed history.
    function handleSessionDeleted(sessionId) {
        setSessions((currentSessions) =>
            currentSessions.filter(
                (session) => session.id !== sessionId
            )
        );

        // If the deleted session was being edited, return the form
        // to its normal create-session state.
        if (editingSessionId === sessionId) {
            resetForm();
            setMessage("");
            setMessageType("info");
        }
    }

    // Check values that require more validation than HTML attributes provide.
    function validateForm() {
        if (!discipline.trim()) {
            return "Please enter a discipline.";
        }

        if (!trainingType.trim()) {
            return "Please enter a training type.";
        }

        if (!sessionDate) {
            return "Please select a session date.";
        }

        if (sessionDate > today) {
            return "Session date cannot be in the future.";
        }

        const parsedDuration = Number(durationMinutes);

        if (
            !Number.isInteger(parsedDuration) ||
            parsedDuration <= 0
        ) {
            return "Duration must be a whole number greater than zero.";
        }

        if (
            recordingMethod !== "manual" &&
            recordingMethod !== "training_mode"
        ) {
            return "Please select a valid recording method.";
        }

        return "";
    }

    // Submit either a new session or edits to an existing session.
    async function handleSubmit(event) {
        event.preventDefault();

        if (submissionInProgress.current) {
            return;
        }

        const validationMessage = validateForm();

        if (validationMessage) {
            setMessageType("error");
            setMessage(validationMessage);
            return;
        }

        submissionInProgress.current = true;
        setMessage("");
        setLoading(true);

        const sessionValues = {
            discipline: discipline.trim(),
            trainingType: trainingType.trim(),
            sessionDate,
            durationMinutes: Number(durationMinutes),
            recordingMethod,
            notes: notes.trim()
        };

        try {
            // Edit mode sends the complete form to the PUT route.
            if (editingSessionId !== null) {
                const result = await updateCombatSportsSession(
                    editingSessionId,
                    sessionValues
                );

                setMessageType(
                    result.success ? "success" : "error"
                );

                if (!result.success) {
                    setMessage(
                        result.message ||
                        "Unable to update the session."
                    );
                    return;
                }

                // Fall back to the submitted values if a successful
                // response unexpectedly omits its session object.
                const updatedSession = result.session || {
                    id: editingSessionId,
                    discipline: sessionValues.discipline,
                    training_type: sessionValues.trainingType,
                    session_date: sessionValues.sessionDate,
                    duration_minutes:
                        sessionValues.durationMinutes,
                    recording_method:
                        sessionValues.recordingMethod,
                    notes: sessionValues.notes
                };

                // Preserve fields such as created_at while replacing
                // the editable values returned by the PUT route.
                setSessions((currentSessions) =>
                    sortSessionsNewestFirst(
                        currentSessions.map((session) =>
                            session.id === editingSessionId
                                ? {
                                    ...session,
                                    ...updatedSession
                                }
                                : session
                        )
                    )
                );

                setMessage(
                    `${updatedSession.discipline} session updated successfully.`
                );
                setHistoryError("");
                resetForm();
                return;
            }

            // Create mode sends a new session to the POST route.
            const result = await createCombatSportsSession(
                sessionValues
            );

            setMessageType(
                result.success ? "success" : "error"
            );

            if (!result.success) {
                setMessage(
                    result.message ||
                    "Unable to record the session."
                );
                return;
            }

            const savedDiscipline =
                result.session?.discipline ||
                sessionValues.discipline;

            setMessage(
                `${savedDiscipline} session created successfully.`
            );

            // Add the new session to history immediately.
            if (result.session) {
                setSessions((currentSessions) =>
                    sortSessionsNewestFirst([
                        result.session,
                        ...currentSessions
                    ])
                );
                setHistoryError("");
            }

            resetForm();
        } catch {
            setMessageType("error");

            setMessage(
                editingSessionId !== null
                    ? "Unable to update the session. Please try again."
                    : "Unable to record the session. Please try again."
            );
        } finally {
            submissionInProgress.current = false;
            setLoading(false);
        }
    }

    const editing = editingSessionId !== null;

    return (
        <div className="combat-sports-page">
            <Card
                as="section"
                className="combat-sports-form-card"
                shadow
            >
                <h1>
                    {editing
                        ? "Edit Combat Sports Session"
                        : "Record Combat Sports Session"}
                </h1>

                <p className="form-description">
                    {editing
                        ? "Update the details for the selected session."
                        : "Add the general details from a completed training session."}
                </p>

                <form onSubmit={handleSubmit}>
                    <FormField
                        id="discipline"
                        label="Discipline"
                        type="text"
                        value={discipline}
                        onChange={(event) =>
                            setDiscipline(event.target.value)
                        }
                        disabled={loading}
                        list="discipline-suggestions"
                        placeholder="Select or enter a discipline"
                        autoComplete="off"
                        required
                    />

                    <datalist id="discipline-suggestions">
                        {disciplineSuggestions.map((suggestion) => (
                            <option
                                key={suggestion}
                                value={suggestion}
                            />
                        ))}
                    </datalist>

                    <FormField
                        id="training-type"
                        label="Training Type"
                        type="text"
                        value={trainingType}
                        onChange={(event) =>
                            setTrainingType(event.target.value)
                        }
                        disabled={loading}
                        list="training-type-suggestions"
                        placeholder="Select or enter a training type"
                        autoComplete="off"
                        required
                    />

                    <datalist id="training-type-suggestions">
                        {trainingTypeSuggestions.map((suggestion) => (
                            <option
                                key={suggestion}
                                value={suggestion}
                            />
                        ))}
                    </datalist>

                    <FormField
                        id="session-date"
                        label="Session Date"
                        type="date"
                        value={sessionDate}
                        onChange={(event) =>
                            setSessionDate(event.target.value)
                        }
                        disabled={loading}
                        max={today}
                        required
                    />

                    <FormField
                        id="duration-minutes"
                        label="Duration in Minutes"
                        type="number"
                        value={durationMinutes}
                        onChange={(event) =>
                            setDurationMinutes(event.target.value)
                        }
                        disabled={loading}
                        min="1"
                        step="1"
                        placeholder="For example, 60"
                        required
                    />

                    <SelectField
                        id="recording-method"
                        label="Recording Method"
                        value={recordingMethod}
                        onChange={(event) =>
                            setRecordingMethod(event.target.value)
                        }
                        disabled={loading}
                        required
                    >
                        <option value="manual">Manual</option>

                        <option value="training_mode">
                            Training Mode
                        </option>
                    </SelectField>

                    <TextAreaField
                        id="notes"
                        label="Notes (Optional)"
                        value={notes}
                        onChange={(event) =>
                            setNotes(event.target.value)
                        }
                        disabled={loading}
                        placeholder="Add any useful details about the session"
                        rows="5"
                    />

                    <div className="combat-sports-form-actions">
                        <Button
                            type="submit"
                            loading={loading}
                            loadingText={
                                editing
                                    ? "Saving changes..."
                                    : "Saving session..."
                            }
                        >
                            {editing
                                ? "Save Changes"
                                : "Save Session"}
                        </Button>

                        {editing && (
                            <Button
                                type="button"
                                variant="secondary"
                                onClick={cancelEditingSession}
                                disabled={loading}
                            >
                                Cancel
                            </Button>
                        )}
                    </div>
                </form>

                <FeedbackMessage type={messageType}>
                    {message}
                </FeedbackMessage>
            </Card>

            <CombatSportsSessionHistory
                sessions={sessions}
                loading={historyLoading}
                error={historyError}
                onEditSession={beginEditingSession}
                onSessionDeleted={handleSessionDeleted}
                editingSessionId={editingSessionId}
            />

            <div className="combat-sports-content-area">
                <CombatSportsContentManager />
            </div>
        </div>
    );
}

export default CombatSports;
