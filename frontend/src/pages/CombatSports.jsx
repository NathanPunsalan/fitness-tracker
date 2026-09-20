import { useRef, useState } from "react";

import Button from "../components/ui/Button";
import Card from "../components/ui/Card";
import FeedbackMessage from "../components/ui/FeedbackMessage";
import FormField from "../components/ui/FormField";
import SelectField from "../components/ui/SelectField";
import TextAreaField from "../components/ui/TextAreaField";
import {
    createCombatSportsSession
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

    // Store the text and visual type of the latest feedback message.
    const [message, setMessage] = useState("");
    const [messageType, setMessageType] = useState("info");

    // Track whether a session request is currently processing.
    const [loading, setLoading] = useState(false);

    // This ref changes immediately, preventing rapid duplicate submissions
    // before React has time to apply the loading-state update.
    const submissionInProgress = useRef(false);

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
                // Prefer the discipline returned by the backend so the
                // confirmation describes the session that was saved.
                const savedDiscipline =
                    result.session?.discipline ||
                    discipline.trim();

                setMessage(
                    `${savedDiscipline} session created successfully.`
                );

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
        <Card
            as="section"
            className="combat-sports-form-card"
            shadow
        >
            <h1>Record Combat Sports Session</h1>

            <p className="form-description">
                Add the general details from a completed training session.
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
    );
}

export default CombatSports;
