import { useRef, useState } from "react";

import Button from "../ui/Button";
import FeedbackMessage from "../ui/FeedbackMessage";
import FormField from "../ui/FormField";
import TextAreaField from "../ui/TextAreaField";

import {
    createCombatSportsTechnique,
    deleteCombatSportsTechnique,
    updateCombatSportsTechnique
} from "../../services/combatSportsService";

const emptyForm = {
    discipline: "",
    name: "",
    category: "",
    description: ""
};

function TechniqueManager({ techniques, onSaved, onDeleted }) {
    const [form, setForm] = useState(emptyForm);
    const [editingId, setEditingId] = useState(null);
    const [loading, setLoading] = useState(false);
    const [deletingId, setDeletingId] = useState(null);
    const [message, setMessage] = useState("");
    const [messageType, setMessageType] = useState("info");
    const requestInProgress = useRef(false);
    const disciplineSuggestions = [...new Set(
        techniques.map((technique) => technique.discipline)
    )];

    function updateField(field, value) {
        setForm((current) => ({ ...current, [field]: value }));
    }

    function resetForm() {
        setForm(emptyForm);
        setEditingId(null);
    }

    function beginEditing(technique) {
        setForm({
            discipline: technique.discipline,
            name: technique.name,
            category: technique.category || "",
            description: technique.description || ""
        });
        setEditingId(technique.id);
        setMessage("");
    }

    async function handleSubmit(event) {
        event.preventDefault();

        if (requestInProgress.current) {
            return;
        }

        if (!form.discipline.trim() || !form.name.trim()) {
            setMessageType("error");
            setMessage("Discipline and technique name are required.");
            return;
        }

        requestInProgress.current = true;
        setLoading(true);
        setMessage("");

        const values = {
            discipline: form.discipline.trim(),
            name: form.name.trim(),
            category: form.category.trim(),
            description: form.description.trim()
        };

        try {
            const result = editingId === null
                ? await createCombatSportsTechnique(values)
                : await updateCombatSportsTechnique(editingId, values);

            setMessageType(result.success ? "success" : "error");

            if (!result.success || !result.technique) {
                setMessage(result.message || "Unable to save the technique.");
                return;
            }

            onSaved(result.technique);
            setMessage(
                `${result.technique.name} ${
                    editingId === null ? "created" : "updated"
                } successfully.`
            );
            resetForm();
        } catch {
            setMessageType("error");
            setMessage("Unable to save the technique. Please try again.");
        } finally {
            requestInProgress.current = false;
            setLoading(false);
        }
    }

    async function handleDelete(technique) {
        const confirmed = window.confirm(
            `Delete ${technique.name}? Referenced techniques cannot be deleted.`
        );

        if (!confirmed || deletingId !== null) {
            return;
        }

        setDeletingId(technique.id);
        setMessage("");

        try {
            const result = await deleteCombatSportsTechnique(technique.id);
            setMessageType(result.success ? "success" : "error");

            if (!result.success) {
                setMessage(result.message || "Unable to delete the technique.");
                return;
            }

            onDeleted(technique.id);
            if (editingId === technique.id) {
                resetForm();
            }
            setMessage(`${technique.name} deleted successfully.`);
        } catch {
            setMessageType("error");
            setMessage("Unable to delete the technique. Please try again.");
        } finally {
            setDeletingId(null);
        }
    }

    return (
        <div className="library-manager-grid">
            <form className="library-form" onSubmit={handleSubmit}>
                <h3>{editingId === null ? "Add Technique" : "Edit Technique"}</h3>

                <FormField
                    id="technique-discipline"
                    label="Discipline"
                    value={form.discipline}
                    onChange={(event) => updateField("discipline", event.target.value)}
                    disabled={loading}
                    placeholder="For example, Muay Thai"
                    list="technique-discipline-suggestions"
                    autoComplete="off"
                    required
                />

                <datalist id="technique-discipline-suggestions">
                    {disciplineSuggestions.map((suggestion) => (
                        <option key={suggestion} value={suggestion} />
                    ))}
                </datalist>

                <FormField
                    id="technique-name"
                    label="Technique Name"
                    value={form.name}
                    onChange={(event) => updateField("name", event.target.value)}
                    disabled={loading}
                    placeholder="For example, Jab"
                    required
                />

                <FormField
                    id="technique-category"
                    label="Category (Optional)"
                    value={form.category}
                    onChange={(event) => updateField("category", event.target.value)}
                    disabled={loading}
                    placeholder="Punch, kick, guard, takedown..."
                />

                <TextAreaField
                    id="technique-description"
                    label="Description (Optional)"
                    value={form.description}
                    onChange={(event) => updateField("description", event.target.value)}
                    disabled={loading}
                    rows="4"
                />

                <div className="library-form__actions">
                    <Button type="submit" loading={loading} loadingText="Saving...">
                        {editingId === null ? "Add Technique" : "Save Changes"}
                    </Button>

                    {editingId !== null && (
                        <Button type="button" variant="secondary" onClick={resetForm} disabled={loading}>
                            Cancel
                        </Button>
                    )}
                </div>

                <FeedbackMessage type={messageType}>{message}</FeedbackMessage>
            </form>

            <div className="library-list">
                <h3>Saved Techniques</h3>

                {techniques.length === 0 ? (
                    <p className="library-empty">No techniques saved yet.</p>
                ) : techniques.map((technique) => (
                    <article className="library-item" key={technique.id}>
                        <div className="library-item__header">
                            <div>
                                <h4>{technique.name}</h4>
                                <p>{technique.discipline}{technique.category ? ` · ${technique.category}` : ""}</p>
                            </div>
                            <div className="library-item__actions">
                                <Button type="button" variant="secondary" onClick={() => beginEditing(technique)} disabled={deletingId !== null}>
                                    Edit
                                </Button>
                                <Button type="button" variant="danger" loading={deletingId === technique.id} loadingText="Deleting..." onClick={() => handleDelete(technique)} disabled={deletingId !== null && deletingId !== technique.id}>
                                    Delete
                                </Button>
                            </div>
                        </div>
                        {technique.description && <p className="library-item__description">{technique.description}</p>}
                    </article>
                ))}
            </div>
        </div>
    );
}

export default TechniqueManager;
