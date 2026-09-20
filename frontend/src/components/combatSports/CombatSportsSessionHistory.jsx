import Card from "../ui/Card";
import FeedbackMessage from "../ui/FeedbackMessage";
import LoadingIndicator from "../ui/LoadingIndicator";

import "./CombatSportsSessionHistory.css";

// Convert the stored YYYY-MM-DD value into a readable local date.
// Splitting the date manually avoids timezone changes shifting the day.
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

// Convert database values such as "training_mode" into readable text.
function formatRecordingMethod(recordingMethod) {
    if (recordingMethod === "training_mode") {
        return "Training Mode";
    }

    return "Manual";
}

function CombatSportsSessionHistory({
    sessions,
    loading,
    error
}) {
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
                        {sessions.map((session) => (
                            <article
                                className="combat-sports-session"
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
                                            {session.duration_minutes} minutes
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
                            </article>
                        ))}
                    </div>
                )}
        </Card>
    );
}

export default CombatSportsSessionHistory;