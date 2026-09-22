import { useMemo, useRef, useState } from "react";

import Button from "../ui/Button";
import FeedbackMessage from "../ui/FeedbackMessage";
import FormField from "../ui/FormField";
import SelectField from "../ui/SelectField";
import TextAreaField from "../ui/TextAreaField";

import {
    createCombatSportsCombination,
    deleteCombatSportsCombination,
    updateCombatSportsCombination
} from "../../services/combatSportsService";

function normalize(value) {
    // SQLite discipline matching is case-sensitive, so keep the comparison
    // identical to the value the API will validate.
    return value.trim();
}

function moveItem(items, index, direction) {
    const nextIndex = index + direction;
    if (nextIndex < 0 || nextIndex >= items.length) {
        return items;
    }

    const updatedItems = [...items];
    [updatedItems[index], updatedItems[nextIndex]] =
        [updatedItems[nextIndex], updatedItems[index]];
    return updatedItems;
}

function CombinationManager({ techniques, combinations, onSaved, onDeleted }) {
    const [discipline, setDiscipline] = useState("");
    const [name, setName] = useState("");
    const [description, setDescription] = useState("");
    const [techniqueIds, setTechniqueIds] = useState([]);
    const [selectedTechniqueId, setSelectedTechniqueId] = useState("");
    const [editingId, setEditingId] = useState(null);
    const [loading, setLoading] = useState(false);
    const [deletingId, setDeletingId] = useState(null);
    const [message, setMessage] = useState("");
    const [messageType, setMessageType] = useState("info");
    const requestInProgress = useRef(false);

    const eligibleTechniques = useMemo(() => {
        const selectedDiscipline = normalize(discipline);
        return selectedDiscipline
            ? techniques.filter((technique) =>
                normalize(technique.discipline) === selectedDiscipline
            )
            : [];
    }, [discipline, techniques]);

    const techniquesById = useMemo(
        () => new Map(techniques.map((technique) => [technique.id, technique])),
        [techniques]
    );
    const disciplineSuggestions = useMemo(
        () => [...new Set(techniques.map((technique) => technique.discipline))],
        [techniques]
    );

    function resetForm() {
        setDiscipline("");
        setName("");
        setDescription("");
        setTechniqueIds([]);
        setSelectedTechniqueId("");
        setEditingId(null);
    }

    function beginEditing(combination) {
        setDiscipline(combination.discipline);
        setName(combination.name);
        setDescription(combination.description || "");
        setTechniqueIds(combination.steps.map((step) => step.technique_id));
        setSelectedTechniqueId("");
        setEditingId(combination.id);
        setMessage("");
    }

    function addTechnique() {
        const parsedId = Number(selectedTechniqueId);
        if (!Number.isInteger(parsedId)) {
            return;
        }
        setTechniqueIds((ids) => [...ids, parsedId]);
        setSelectedTechniqueId("");
    }

    async function handleSubmit(event) {
        event.preventDefault();
        if (requestInProgress.current) return;

        if (!discipline.trim() || !name.trim()) {
            setMessageType("error");
            setMessage("Discipline and combination name are required.");
            return;
        }
        if (techniqueIds.length === 0) {
            setMessageType("error");
            setMessage("Add at least one technique to the combination.");
            return;
        }
        if (techniqueIds.some((id) =>
            normalize(techniquesById.get(id)?.discipline || "") !== normalize(discipline)
        )) {
            setMessageType("error");
            setMessage("Every technique must match the combination discipline.");
            return;
        }

        requestInProgress.current = true;
        setLoading(true);
        setMessage("");

        const values = {
            discipline: discipline.trim(),
            name: name.trim(),
            description: description.trim(),
            technique_ids: techniqueIds
        };

        try {
            const result = editingId === null
                ? await createCombatSportsCombination(values)
                : await updateCombatSportsCombination(editingId, values);
            setMessageType(result.success ? "success" : "error");
            if (!result.success || !result.combination) {
                setMessage(result.message || "Unable to save the combination.");
                return;
            }
            onSaved(result.combination);
            setMessage(`${result.combination.name} ${editingId === null ? "created" : "updated"} successfully.`);
            resetForm();
        } catch {
            setMessageType("error");
            setMessage("Unable to save the combination. Please try again.");
        } finally {
            requestInProgress.current = false;
            setLoading(false);
        }
    }

    async function handleDelete(combination) {
        if (!window.confirm(`Delete ${combination.name}? Drills using it must be changed first.`) || deletingId !== null) return;
        setDeletingId(combination.id);
        setMessage("");
        try {
            const result = await deleteCombatSportsCombination(combination.id);
            setMessageType(result.success ? "success" : "error");
            if (!result.success) {
                setMessage(result.message || "Unable to delete the combination.");
                return;
            }
            onDeleted(combination.id);
            if (editingId === combination.id) resetForm();
            setMessage(`${combination.name} deleted successfully.`);
        } catch {
            setMessageType("error");
            setMessage("Unable to delete the combination. Please try again.");
        } finally {
            setDeletingId(null);
        }
    }

    return (
        <div className="library-manager-grid">
            <form className="library-form" onSubmit={handleSubmit}>
                <h3>{editingId === null ? "Build Combination" : "Edit Combination"}</h3>
                <FormField id="combination-discipline" label="Discipline" value={discipline} onChange={(event) => setDiscipline(event.target.value)} disabled={loading} placeholder="Enter a saved technique discipline" list="combination-discipline-suggestions" autoComplete="off" required />
                <datalist id="combination-discipline-suggestions">
                    {disciplineSuggestions.map((suggestion) => <option key={suggestion} value={suggestion} />)}
                </datalist>
                <FormField id="combination-name" label="Combination Name" value={name} onChange={(event) => setName(event.target.value)} disabled={loading} required />
                <TextAreaField id="combination-description" label="Description (Optional)" value={description} onChange={(event) => setDescription(event.target.value)} disabled={loading} rows="3" />

                <div className="library-builder">
                    <h4>Ordered Steps</h4>
                    <div className="library-builder__add">
                        <SelectField id="combination-technique" label="Technique" value={selectedTechniqueId} onChange={(event) => setSelectedTechniqueId(event.target.value)} disabled={loading || eligibleTechniques.length === 0}>
                            <option value="">Select a technique</option>
                            {eligibleTechniques.map((technique) => <option key={technique.id} value={technique.id}>{technique.name}</option>)}
                        </SelectField>
                        <Button type="button" variant="secondary" onClick={addTechnique} disabled={loading || !selectedTechniqueId}>Add Step</Button>
                    </div>
                    {discipline && eligibleTechniques.length === 0 && <p className="library-helper">Create a technique with this exact discipline first.</p>}
                    <ol className="ordered-items">
                        {techniqueIds.map((id, index) => (
                            <li key={`${id}-${index}`}>
                                <span>{techniquesById.get(id)?.name || "Unavailable technique"}</span>
                                <div>
                                    <button type="button" onClick={() => setTechniqueIds((ids) => moveItem(ids, index, -1))} disabled={loading || index === 0} aria-label="Move step up">↑</button>
                                    <button type="button" onClick={() => setTechniqueIds((ids) => moveItem(ids, index, 1))} disabled={loading || index === techniqueIds.length - 1} aria-label="Move step down">↓</button>
                                    <button type="button" onClick={() => setTechniqueIds((ids) => ids.filter((_, itemIndex) => itemIndex !== index))} disabled={loading} aria-label="Remove step">Remove</button>
                                </div>
                            </li>
                        ))}
                    </ol>
                </div>

                <div className="library-form__actions">
                    <Button type="submit" loading={loading} loadingText="Saving...">{editingId === null ? "Create Combination" : "Save Changes"}</Button>
                    {editingId !== null && <Button type="button" variant="secondary" onClick={resetForm} disabled={loading}>Cancel</Button>}
                </div>
                <FeedbackMessage type={messageType}>{message}</FeedbackMessage>
            </form>

            <div className="library-list">
                <h3>Saved Combinations</h3>
                {combinations.length === 0 ? <p className="library-empty">No combinations saved yet.</p> : combinations.map((combination) => (
                    <article className="library-item" key={combination.id}>
                        <div className="library-item__header">
                            <div><h4>{combination.name}</h4><p>{combination.discipline}</p></div>
                            <div className="library-item__actions">
                                <Button type="button" variant="secondary" onClick={() => beginEditing(combination)} disabled={deletingId !== null}>Edit</Button>
                                <Button type="button" variant="danger" loading={deletingId === combination.id} loadingText="Deleting..." onClick={() => handleDelete(combination)} disabled={deletingId !== null && deletingId !== combination.id}>Delete</Button>
                            </div>
                        </div>
                        <p className="library-sequence">{combination.steps.map((step) => step.technique_name).join(" → ")}</p>
                        {combination.description && <p className="library-item__description">{combination.description}</p>}
                    </article>
                ))}
            </div>
        </div>
    );
}

export default CombinationManager;
