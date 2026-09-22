import { useMemo, useRef, useState } from "react";

import Button from "../ui/Button";
import FeedbackMessage from "../ui/FeedbackMessage";
import FormField from "../ui/FormField";
import SelectField from "../ui/SelectField";
import TextAreaField from "../ui/TextAreaField";

import {
    createCombatSportsDrill,
    deleteCombatSportsDrill,
    updateCombatSportsDrill
} from "../../services/combatSportsService";

function normalize(value) {
    // Match the backend's case-sensitive discipline comparison exactly.
    return value.trim();
}

function moveItem(items, index, direction) {
    const nextIndex = index + direction;
    if (nextIndex < 0 || nextIndex >= items.length) return items;
    const updated = [...items];
    [updated[index], updated[nextIndex]] = [updated[nextIndex], updated[index]];
    return updated;
}

function optionalPositiveInteger(value) {
    if (value === "") return null;
    const parsed = Number(value);
    return Number.isInteger(parsed) && parsed > 0 ? parsed : undefined;
}

function DrillManager({ techniques, combinations, drills, onSaved, onDeleted }) {
    const [discipline, setDiscipline] = useState("");
    const [name, setName] = useState("");
    const [instructions, setInstructions] = useState("");
    const [duration, setDuration] = useState("");
    const [repetitions, setRepetitions] = useState("");
    const [rounds, setRounds] = useState("");
    const [notes, setNotes] = useState("");
    const [items, setItems] = useState([]);
    const [itemType, setItemType] = useState("technique");
    const [referenceId, setReferenceId] = useState("");
    const [editingId, setEditingId] = useState(null);
    const [loading, setLoading] = useState(false);
    const [deletingId, setDeletingId] = useState(null);
    const [message, setMessage] = useState("");
    const [messageType, setMessageType] = useState("info");
    const requestInProgress = useRef(false);

    const eligibleReferences = useMemo(() => {
        const normalizedDiscipline = normalize(discipline);
        if (!normalizedDiscipline) return [];
        const source = itemType === "technique" ? techniques : combinations;
        return source.filter((item) => normalize(item.discipline) === normalizedDiscipline);
    }, [combinations, discipline, itemType, techniques]);
    const disciplineSuggestions = useMemo(
        () => [...new Set([
            ...techniques.map((technique) => technique.discipline),
            ...combinations.map((combination) => combination.discipline)
        ])],
        [combinations, techniques]
    );

    function resetForm() {
        setDiscipline(""); setName(""); setInstructions("");
        setDuration(""); setRepetitions(""); setRounds("");
        setNotes(""); setItems([]); setItemType("technique");
        setReferenceId(""); setEditingId(null);
    }

    function beginEditing(drill) {
        setDiscipline(drill.discipline);
        setName(drill.name);
        setInstructions(drill.instructions || "");
        setDuration(drill.default_duration_seconds ?? "");
        setRepetitions(drill.default_repetitions ?? "");
        setRounds(drill.default_rounds ?? "");
        setNotes(drill.notes || "");
        setItems(drill.items.map((item) => ({
            item_type: item.item_type,
            reference_id: item.item_type === "technique"
                ? item.technique_id
                : item.combination_id,
            item_name: item.item_name
        })));
        setEditingId(drill.id);
        setMessage("");
    }

    function addItem() {
        const parsedId = Number(referenceId);
        const selected = eligibleReferences.find((item) => item.id === parsedId);
        if (!selected) return;
        setItems((current) => [...current, {
            item_type: itemType,
            reference_id: parsedId,
            item_name: selected.name
        }]);
        setReferenceId("");
    }

    async function handleSubmit(event) {
        event.preventDefault();
        if (requestInProgress.current) return;

        const parsedDuration = optionalPositiveInteger(duration);
        const parsedRepetitions = optionalPositiveInteger(repetitions);
        const parsedRounds = optionalPositiveInteger(rounds);

        if (!discipline.trim() || !name.trim()) {
            setMessageType("error");
            setMessage("Discipline and drill name are required.");
            return;
        }
        if ([parsedDuration, parsedRepetitions, parsedRounds].includes(undefined)) {
            setMessageType("error");
            setMessage("Drill defaults must be positive whole numbers when provided.");
            return;
        }
        if (items.length === 0) {
            setMessageType("error");
            setMessage("Add at least one technique or combination to the drill.");
            return;
        }

        requestInProgress.current = true;
        setLoading(true);
        setMessage("");

        const values = {
            discipline: discipline.trim(),
            name: name.trim(),
            instructions: instructions.trim(),
            default_duration_seconds: parsedDuration,
            default_repetitions: parsedRepetitions,
            default_rounds: parsedRounds,
            notes: notes.trim(),
            items: items.map(({ item_type, reference_id }) => ({
                item_type,
                reference_id
            }))
        };

        try {
            const result = editingId === null
                ? await createCombatSportsDrill(values)
                : await updateCombatSportsDrill(editingId, values);
            setMessageType(result.success ? "success" : "error");
            if (!result.success || !result.drill) {
                setMessage(result.message || "Unable to save the drill.");
                return;
            }
            onSaved(result.drill);
            setMessage(`${result.drill.name} ${editingId === null ? "created" : "updated"} successfully.`);
            resetForm();
        } catch {
            setMessageType("error");
            setMessage("Unable to save the drill. Please try again.");
        } finally {
            requestInProgress.current = false;
            setLoading(false);
        }
    }

    async function handleDelete(drill) {
        if (!window.confirm(`Delete ${drill.name}?`) || deletingId !== null) return;
        setDeletingId(drill.id);
        setMessage("");
        try {
            const result = await deleteCombatSportsDrill(drill.id);
            setMessageType(result.success ? "success" : "error");
            if (!result.success) {
                setMessage(result.message || "Unable to delete the drill.");
                return;
            }
            onDeleted(drill.id);
            if (editingId === drill.id) resetForm();
            setMessage(`${drill.name} deleted successfully.`);
        } catch {
            setMessageType("error");
            setMessage("Unable to delete the drill. Please try again.");
        } finally {
            setDeletingId(null);
        }
    }

    return (
        <div className="library-manager-grid">
            <form className="library-form" onSubmit={handleSubmit}>
                <h3>{editingId === null ? "Build Drill" : "Edit Drill"}</h3>
                <FormField id="drill-discipline" label="Discipline" value={discipline} onChange={(event) => setDiscipline(event.target.value)} disabled={loading} list="drill-discipline-suggestions" autoComplete="off" required />
                <datalist id="drill-discipline-suggestions">
                    {disciplineSuggestions.map((suggestion) => <option key={suggestion} value={suggestion} />)}
                </datalist>
                <FormField id="drill-name" label="Drill Name" value={name} onChange={(event) => setName(event.target.value)} disabled={loading} required />
                <TextAreaField id="drill-instructions" label="Instructions (Optional)" value={instructions} onChange={(event) => setInstructions(event.target.value)} disabled={loading} rows="3" />

                <div className="library-number-grid">
                    <FormField id="drill-duration" label="Seconds (Optional)" type="number" min="1" step="1" value={duration} onChange={(event) => setDuration(event.target.value)} disabled={loading} />
                    <FormField id="drill-repetitions" label="Reps (Optional)" type="number" min="1" step="1" value={repetitions} onChange={(event) => setRepetitions(event.target.value)} disabled={loading} />
                    <FormField id="drill-rounds" label="Rounds (Optional)" type="number" min="1" step="1" value={rounds} onChange={(event) => setRounds(event.target.value)} disabled={loading} />
                </div>

                <div className="library-builder">
                    <h4>Ordered Drill Items</h4>
                    <div className="library-builder__type">
                        <SelectField id="drill-item-type" label="Item Type" value={itemType} onChange={(event) => { setItemType(event.target.value); setReferenceId(""); }} disabled={loading}>
                            <option value="technique">Technique</option>
                            <option value="combination">Combination</option>
                        </SelectField>
                        <SelectField id="drill-reference" label={itemType === "technique" ? "Technique" : "Combination"} value={referenceId} onChange={(event) => setReferenceId(event.target.value)} disabled={loading || eligibleReferences.length === 0}>
                            <option value="">Select content</option>
                            {eligibleReferences.map((item) => <option key={item.id} value={item.id}>{item.name}</option>)}
                        </SelectField>
                    </div>
                    <Button type="button" variant="secondary" onClick={addItem} disabled={loading || !referenceId}>Add Item</Button>
                    {discipline && eligibleReferences.length === 0 && <p className="library-helper">No matching {itemType}s use this discipline yet.</p>}
                    <ol className="ordered-items">
                        {items.map((item, index) => (
                            <li key={`${item.item_type}-${item.reference_id}-${index}`}>
                                <span><small>{item.item_type}</small>{item.item_name}</span>
                                <div>
                                    <button type="button" onClick={() => setItems((current) => moveItem(current, index, -1))} disabled={loading || index === 0} aria-label="Move item up">↑</button>
                                    <button type="button" onClick={() => setItems((current) => moveItem(current, index, 1))} disabled={loading || index === items.length - 1} aria-label="Move item down">↓</button>
                                    <button type="button" onClick={() => setItems((current) => current.filter((_, itemIndex) => itemIndex !== index))} disabled={loading} aria-label="Remove item">Remove</button>
                                </div>
                            </li>
                        ))}
                    </ol>
                </div>

                <TextAreaField id="drill-notes" label="Notes (Optional)" value={notes} onChange={(event) => setNotes(event.target.value)} disabled={loading} rows="3" />
                <div className="library-form__actions">
                    <Button type="submit" loading={loading} loadingText="Saving...">{editingId === null ? "Create Drill" : "Save Changes"}</Button>
                    {editingId !== null && <Button type="button" variant="secondary" onClick={resetForm} disabled={loading}>Cancel</Button>}
                </div>
                <FeedbackMessage type={messageType}>{message}</FeedbackMessage>
            </form>

            <div className="library-list">
                <h3>Saved Drills</h3>
                {drills.length === 0 ? <p className="library-empty">No drills saved yet.</p> : drills.map((drill) => (
                    <article className="library-item" key={drill.id}>
                        <div className="library-item__header">
                            <div><h4>{drill.name}</h4><p>{drill.discipline}</p></div>
                            <div className="library-item__actions">
                                <Button type="button" variant="secondary" onClick={() => beginEditing(drill)} disabled={deletingId !== null}>Edit</Button>
                                <Button type="button" variant="danger" loading={deletingId === drill.id} loadingText="Deleting..." onClick={() => handleDelete(drill)} disabled={deletingId !== null && deletingId !== drill.id}>Delete</Button>
                            </div>
                        </div>
                        <p className="library-sequence">{drill.items.map((item) => item.item_name).join(" → ")}</p>
                        <div className="library-defaults">
                            {drill.default_duration_seconds && <span>{drill.default_duration_seconds}s</span>}
                            {drill.default_repetitions && <span>{drill.default_repetitions} reps</span>}
                            {drill.default_rounds && <span>{drill.default_rounds} rounds</span>}
                        </div>
                        {drill.instructions && <p className="library-item__description">{drill.instructions}</p>}
                    </article>
                ))}
            </div>
        </div>
    );
}

export default DrillManager;
