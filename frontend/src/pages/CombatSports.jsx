import {
    useEffect,
    useRef,
    useState
} from "react";

import CombatSportsSessionHistory from "../components/combatSports/CombatSportsSessionHistory";
import Button from "../components/ui/Button";
import Card from "../components/ui/Card";
import FeedbackMessage from "../components/ui/FeedbackMessage";
import FormField from "../components/ui/FormField";
import SelectField from "../components/ui/SelectField";
import TextAreaField from "../components/ui/TextAreaField";

import {
    createCombatSportsSession,
    getCombatSportsSessions
} from "../services/combatSportsService";

// Suggested values help keep common entries consistent while still
// allowing the user to enter a discipline or training type not listed.
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

// Produce today's date in the user's local timezone for the date input.
function getTodayDate() {
    const today = new Date();
    const year = today.getFullYear();
    const month = String(today.getMonth() + 1).padStart(2, "0");
    const day = String(today.getDate()).padStart(2, "0");

    return `${year}-${month}-${day}`;
}

// Keep newly created sessions in the same newest-first order
// used by the backend database query.
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

    // Store the current value of each session field.
    const [discipline, setDiscipline] = useState("");
    const [trainingType, setTrainingType] = useState("");
    const [sessionDate, setSessionDate] = useState(today);
    const [durationMinutes, setDurationMinutes] = useState("");
    const [recordingMethod, setRecordingMethod] =
        useState("manual");
    const [notes, setNotes] = useState("");

    // Store the text and visual type of the latest form message.
    const [message, setMessage] = useState("");
    const [messageType, setMessageType] = useState("info");

    // Track whether a session creation request is processing.
    const [loading, setLoading] = useState(false);

    // Store session history separately from the form state.
    const [sessions, setSessions] = useState([]);
    const [historyLoading, setHistoryLoading] = useState(true);
    const [historyError, setHistoryError] = useState("");

    // This ref changes immediately, preventing rapid duplicate submissions
    // before React has time to apply the loading-state update.
    const submissionInProgress = useRef(false);

    // Load the authenticated user's session history when the page opens.
    useEffect(() => {
        let requestCancelled = false;

        async function loadSessionHistory() {
            try {
                const result = await getCombatSportsSessions();

                // Ignore the result if the page was removed while
                // the request was still processing.
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

    // Submit the completed session to the Crow API.
    async function handleSubmit(event) {
        event.preventDefault();

        // Stop repeated clicks from creating duplicate sessions.
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

        try {
            const result = await createCombatSportsSession({
                discipline: discipline.trim(),
                trainingType: trainingType.trim(),
                sessionDate,
                durationMinutes: Number(durationMinutes),
                recordingMethod,
                notes: notes.trim()
            });

            setMessageType(
                result.success ? "success" : "error"
            );

            if (result.success) {
                const savedDiscipline =
                    result.session?.discipline ||
                    discipline.trim();

                setMessage(
                    `${savedDiscipline} session created successfully.`
                );

                // Add the new session to history immediately so the user
                // does not need to reload the page or send another GET request.
                if (result.session) {
                    setSessions((currentSessions) =>
                        sortSessionsNewestFirst([
                            result.session,
                            ...currentSessions
                        ])
                    );
                    setHistoryError("");
                }

                // Clear session-specific values while keeping helpful defaults.
                setDiscipline("");
                setTrainingType("");
                setSessionDate(today);
                setDurationMinutes("");
                setRecordingMethod("manual");
                setNotes("");
            } else {
                setMessage(
                    result.message ||
                    "Unable to record the session."
                );
            }
        } catch {
            setMessageType("error");
            setMessage(
                "Unable to record the session. Please try again."
            );
        } finally {
            submissionInProgress.current = false;
            setLoading(false);
        }
    }

    return (
        <div className="combat-sports-page">
            <Card
                as="section"
                className="combat-sports-form-card"
                shadow
            >
                <h1>Record Combat Sports Session</h1>

                <p className="form-description">
                    Add the general details from a completed
                    training session.
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

                    <Button
                        type="submit"
                        loading={loading}
                        loadingText="Saving session..."
                    >
                        Save Session
                    </Button>
                </form>

                <FeedbackMessage type={messageType}>
                    {message}
                </FeedbackMessage>
            </Card>

            <CombatSportsSessionHistory
                sessions={sessions}
                loading={historyLoading}
                error={historyError}
            />
        </div>
    );
}

export default CombatSports;
