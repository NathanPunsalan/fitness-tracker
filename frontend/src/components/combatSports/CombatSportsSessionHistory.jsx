import {
    useRef,
    useState
} from "react";

import {
    deleteCombatSportsSession
} from "../../services/combatSportsService";

import Button from "../ui/Button";
import Card from "../ui/Card";
import FeedbackMessage from "../ui/FeedbackMessage";
import LoadingIndicator from "../ui/LoadingIndicator";

import "./CombatSportsSessionHistory.css";

// Convert the stored YYYY-MM-DD value into a readable local date.
function formatSessionDate(sessionDate) {
    const [year, month, day] = sessionDate
        .split("-")
        .map(Number);

    const date = new Date(year, month - 1, day);

    return date.toLocaleDateString(
        undefined,
        {
            year: "numeric",
            month: "short",
            day: "numeric"
        }
    );
}

// Convert database values into readable labels.
function formatRecordingMethod(recordingMethod) {
    if (recordingMethod === "training_mode") {
        return "Training Mode";
    }

    return "Manual";
}

function CombatSportsSessionHistory({
    sessions,
    loading,
    error,
    onEditSession,
    onSessionDeleted,
    editingSessionId
}) {
    // Track which session is awaiting deletion confirmation.
    const [confirmingDeleteId, setConfirmingDeleteId] =
        useState(null);

    // Track the session currently being deleted.
    const [deletingSessionId, setDeletingSessionId] =
        useState(null);

    // Display feedback for delete operations inside the history card.
    const [deleteMessage, setDeleteMessage] = useState("");
    const [deleteMessageType, setDeleteMessageType] =
        useState("info");

    // Prevent rapid clicks from sending duplicate DELETE requests.
    const deletionInProgress = useRef(false);

    function beginDeleteConfirmation(sessionId) {
        setConfirmingDeleteId(sessionId);
        setDeleteMessage("");
        setDeleteMessageType("info");
    }

    function cancelDeleteConfirmation() {
        if (deletionInProgress.current) {
            return;
        }

        setConfirmingDeleteId(null);
    }

    async function confirmDeleteSession(session) {
        if (
            deletionInProgress.current ||
            !onSessionDeleted
        ) {
            return;
        }

        deletionInProgress.current = true;
        setDeletingSessionId(session.id);
        setDeleteMessage("");

        try {
            const result = await deleteCombatSportsSession(
                session.id
            );

            setDeleteMessageType(
                result.success ? "success" : "error"
            );

            if (!result.success) {
                setDeleteMessage(
                    result.message ||
                    "Unable to delete the session."
                );
                return;
            }

            // Tell the parent page to remove the deleted entry
            // from its session-history state.
            onSessionDeleted(session.id);

            setDeleteMessage(
                `${session.discipline} session deleted successfully.`
            );
            setConfirmingDeleteId(null);
        } catch {
            setDeleteMessageType("error");
            setDeleteMessage(
                "Unable to delete the session. Please try again."
            );
        } finally {
            deletionInProgress.current = false;
            setDeletingSessionId(null);
        }
    }

    return (
        <Card
            as="section"
            className="combat-sports-history-card"
            shadow
        >
            <div className="combat-sports-history-card__header">
                <div>
                    <h2>Session History</h2>

                    <p className="form-description">
                        Review your completed combat-sports sessions.
                    </p>
                </div>

                {!loading && !error && (
                    <span className="combat-sports-history-card__count">
                        {sessions.length}{" "}
                        {sessions.length === 1 ? "session" : "sessions"}
                    </span>
                )}
            </div>

            <FeedbackMessage type={deleteMessageType}>
                {deleteMessage}
            </FeedbackMessage>

            {loading && (
                <div className="combat-sports-history-card__loading">
                    <LoadingIndicator label="Loading sessions..." />
                </div>
            )}

            {!loading && error && (
                <FeedbackMessage type="error">
                    {error}
                </FeedbackMessage>
            )}

            {!loading &&
                !error &&
                sessions.length === 0 && (
                    <div className="combat-sports-history-card__empty">
                        <h3>No sessions recorded yet</h3>

                        <p>
                            Complete the form to add your first
                            combat-sports session.
                        </p>
                    </div>
                )}

            {!loading &&
                !error &&
                sessions.length > 0 && (
                    <div className="combat-sports-session-list">
                        {sessions.map((session) => {
                            const isEditing =
                                editingSessionId === session.id;

                            const isConfirmingDelete =
                                confirmingDeleteId === session.id;

                            const isDeleting =
                                deletingSessionId === session.id;

                            const sessionClasses = [
                                "combat-sports-session",
                                isEditing &&
                                    "combat-sports-session--editing"
                            ]
                                .filter(Boolean)
                                .join(" ");

                            return (
                                <article
                                    className={sessionClasses}
                                    key={session.id}
                                >
                                    <div className="combat-sports-session__header">
                                        <div>
                                            <h3>{session.discipline}</h3>

                                            <span className="combat-sports-session__type">
                                                {session.training_type}
                                            </span>
                                        </div>

                                        <time dateTime={session.session_date}>
                                            {formatSessionDate(
                                                session.session_date
                                            )}
                                        </time>
                                    </div>

                                    <dl className="combat-sports-session__details">
                                        <div>
                                            <dt>Duration</dt>
                                            <dd>
                                                {session.duration_minutes}{" "}
                                                minutes
                                            </dd>
                                        </div>

                                        <div>
                                            <dt>Recorded With</dt>
                                            <dd>
                                                {formatRecordingMethod(
                                                    session.recording_method
                                                )}
                                            </dd>
                                        </div>
                                    </dl>

                                    {session.notes && (
                                        <p className="combat-sports-session__notes">
                                            {session.notes}
                                        </p>
                                    )}

                                    {isConfirmingDelete ? (
                                        <div
                                            className="combat-sports-session__confirmation"
                                            role="alertdialog"
                                            aria-labelledby={
                                                `delete-session-${session.id}`
                                            }
                                        >
                                            <p
                                                id={
                                                    `delete-session-${session.id}`
                                                }
                                            >
                                                Delete this{" "}
                                                {session.discipline} session?
                                            </p>

                                            <div className="combat-sports-session__confirmation-actions">
                                                <Button
                                                    variant="danger"
                                                    onClick={() =>
                                                        confirmDeleteSession(
                                                            session
                                                        )
                                                    }
                                                    loading={isDeleting}
                                                    loadingText="Deleting..."
                                                >
                                                    Confirm Delete
                                                </Button>

                                                <Button
                                                    variant="secondary"
                                                    onClick={
                                                        cancelDeleteConfirmation
                                                    }
                                                    disabled={isDeleting}
                                                >
                                                    Cancel
                                                </Button>
                                            </div>
                                        </div>
                                    ) : (
                                        <div className="combat-sports-session__actions">
                                            <Button
                                                variant="secondary"
                                                onClick={() =>
                                                    onEditSession(session)
                                                }
                                                disabled={
                                                    !onEditSession ||
                                                    isEditing ||
                                                    deletingSessionId !== null
                                                }
                                            >
                                                {isEditing
                                                    ? "Editing"
                                                    : "Edit"}
                                            </Button>

                                            <Button
                                                variant="danger"
                                                onClick={() =>
                                                    beginDeleteConfirmation(
                                                        session.id
                                                    )
                                                }
                                                disabled={
                                                    !onSessionDeleted ||
                                                    deletingSessionId !== null
                                                }
                                            >
                                                Delete
                                            </Button>
                                        </div>
                                    )}
                                </article>
                            );
                        })}
                    </div>
                )}
        </Card>
    );
}

export default CombatSportsSessionHistory;
