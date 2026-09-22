import { useEffect, useState } from "react";

import Card from "../ui/Card";
import FeedbackMessage from "../ui/FeedbackMessage";
import LoadingIndicator from "../ui/LoadingIndicator";
import CombinationManager from "./CombinationManager";
import DrillManager from "./DrillManager";
import TechniqueManager from "./TechniqueManager";

import {
    getCombatSportsCombinations,
    getCombatSportsDrills,
    getCombatSportsTechniques
} from "../../services/combatSportsService";

import "./CombatSportsContentManager.css";

const managerSections = [
    { id: "techniques", label: "Techniques" },
    { id: "combinations", label: "Combinations" },
    { id: "drills", label: "Drills" }
];

function replaceOrAdd(items, savedItem) {
    const itemExists = items.some((item) => item.id === savedItem.id);

    return itemExists
        ? items.map((item) =>
            item.id === savedItem.id ? savedItem : item
        )
        : [savedItem, ...items];
}

function CombatSportsContentManager() {
    const [activeSection, setActiveSection] = useState("techniques");
    const [techniques, setTechniques] = useState([]);
    const [combinations, setCombinations] = useState([]);
    const [drills, setDrills] = useState([]);
    const [loading, setLoading] = useState(true);
    const [error, setError] = useState("");

    // Keep embedded names synchronized after an edit so the user does not
    // need to reload before combinations and drills show the new name.
    function handleTechniqueSaved(technique) {
        setTechniques((items) => replaceOrAdd(items, technique));
        setCombinations((items) => items.map((combination) => ({
            ...combination,
            steps: combination.steps.map((step) =>
                step.technique_id === technique.id
                    ? {
                        ...step,
                        technique_name: technique.name,
                        technique_category: technique.category
                    }
                    : step
            )
        })));
        setDrills((items) => items.map((drill) => ({
            ...drill,
            items: drill.items.map((item) =>
                item.technique_id === technique.id
                    ? { ...item, item_name: technique.name }
                    : item
            )
        })));
    }

    function handleCombinationSaved(combination) {
        setCombinations((items) => replaceOrAdd(items, combination));
        setDrills((items) => items.map((drill) => ({
            ...drill,
            items: drill.items.map((item) =>
                item.combination_id === combination.id
                    ? { ...item, item_name: combination.name }
                    : item
            )
        })));
    }

    useEffect(() => {
        let requestCancelled = false;

        async function loadContent() {
            try {
                const results = await Promise.all([
                    getCombatSportsTechniques(),
                    getCombatSportsCombinations(),
                    getCombatSportsDrills()
                ]);

                if (requestCancelled) {
                    return;
                }

                const [techniqueResult, combinationResult, drillResult] =
                    results;

                if (!results.every((result) => result.success)) {
                    const failedResult = results.find(
                        (result) => !result.success
                    );
                    setError(
                        failedResult?.message ||
                        "Unable to load combat-sports content."
                    );
                    return;
                }

                setTechniques(techniqueResult.techniques || []);
                setCombinations(combinationResult.combinations || []);
                setDrills(drillResult.drills || []);
            } catch {
                if (!requestCancelled) {
                    setError(
                        "Unable to load combat-sports content. Please try again."
                    );
                }
            } finally {
                if (!requestCancelled) {
                    setLoading(false);
                }
            }
        }

        loadContent();

        return () => {
            requestCancelled = true;
        };
    }, []);

    return (
        <Card as="section" className="content-manager" shadow>
            <div className="content-manager__header">
                <div>
                    <h2>Training Library</h2>
                    <p className="form-description">
                        Build reusable techniques, combinations, and drills.
                    </p>
                </div>

                <div className="content-manager__counts" aria-label="Library totals">
                    <span>{techniques.length} techniques</span>
                    <span>{combinations.length} combinations</span>
                    <span>{drills.length} drills</span>
                </div>
            </div>

            <div className="content-manager__tabs" role="tablist" aria-label="Training library sections">
                {managerSections.map((section) => (
                    <button
                        key={section.id}
                        type="button"
                        role="tab"
                        aria-selected={activeSection === section.id}
                        className={
                            activeSection === section.id
                                ? "content-manager__tab content-manager__tab--active"
                                : "content-manager__tab"
                        }
                        onClick={() => setActiveSection(section.id)}
                    >
                        {section.label}
                    </button>
                ))}
            </div>

            {loading ? (
                <div className="content-manager__loading">
                    <LoadingIndicator label="Loading training library..." />
                </div>
            ) : error ? (
                <FeedbackMessage type="error">{error}</FeedbackMessage>
            ) : (
                <div role="tabpanel">
                    {activeSection === "techniques" && (
                        <TechniqueManager
                            techniques={techniques}
                            onSaved={handleTechniqueSaved}
                            onDeleted={(techniqueId) =>
                                setTechniques((items) =>
                                    items.filter((item) => item.id !== techniqueId)
                                )
                            }
                        />
                    )}

                    {activeSection === "combinations" && (
                        <CombinationManager
                            techniques={techniques}
                            combinations={combinations}
                            onSaved={handleCombinationSaved}
                            onDeleted={(combinationId) =>
                                setCombinations((items) =>
                                    items.filter((item) => item.id !== combinationId)
                                )
                            }
                        />
                    )}

                    {activeSection === "drills" && (
                        <DrillManager
                            techniques={techniques}
                            combinations={combinations}
                            drills={drills}
                            onSaved={(drill) =>
                                setDrills((items) => replaceOrAdd(items, drill))
                            }
                            onDeleted={(drillId) =>
                                setDrills((items) =>
                                    items.filter((item) => item.id !== drillId)
                                )
                            }
                        />
                    )}
                </div>
            )}
        </Card>
    );
}

export default CombatSportsContentManager;
