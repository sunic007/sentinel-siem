"""Correlation engine — evaluates security rules against incoming events."""

import logging
from dataclasses import dataclass, field
from datetime import datetime
from typing import Optional

logger = logging.getLogger(__name__)


@dataclass
class CorrelationRule:
    id: str
    name: str
    description: str = ""
    severity: str = "medium"
    enabled: bool = True
    query: str = ""  # SPL query or Sigma rule reference
    condition: dict = field(default_factory=dict)
    risk_score: int = 0
    mitre_attack: list[str] = field(default_factory=list)


@dataclass
class NotableEvent:
    id: str
    rule_id: str
    rule_name: str
    severity: str
    title: str
    description: str
    contributing_events: list[dict] = field(default_factory=list)
    risk_score: int = 0
    entities: list[str] = field(default_factory=list)
    created_at: datetime = field(default_factory=datetime.utcnow)


class CorrelationEngine:
    """Core correlation engine that evaluates rules against events."""

    def __init__(self):
        self.rules: list[CorrelationRule] = []
        self._running = False

    def load_rules(self, rules: list[CorrelationRule]) -> None:
        """Load correlation rules."""
        self.rules = [r for r in rules if r.enabled]
        logger.info("Loaded %d active correlation rules", len(self.rules))

    def evaluate(self, events: list[dict]) -> list[NotableEvent]:
        """Evaluate all rules against a batch of events."""
        notable_events: list[NotableEvent] = []

        for rule in self.rules:
            matches = self._evaluate_rule(rule, events)
            if matches:
                notable = NotableEvent(
                    id=f"NE-{rule.id}-{datetime.utcnow().timestamp():.0f}",
                    rule_id=rule.id,
                    rule_name=rule.name,
                    severity=rule.severity,
                    title=f"[{rule.severity.upper()}] {rule.name}",
                    description=rule.description,
                    contributing_events=matches,
                    risk_score=rule.risk_score,
                )
                notable_events.append(notable)
                logger.info("Notable event created: %s", notable.title)

        return notable_events

    def _evaluate_rule(self, rule: CorrelationRule, events: list[dict]) -> list[dict]:
        """Evaluate a single rule against events."""
        # TODO: Implement rule evaluation logic
        # This will integrate with the SPL engine for query-based rules
        # and pySigma for Sigma rule evaluation
        return []

    def start(self) -> None:
        """Start the correlation engine."""
        self._running = True
        logger.info("Correlation engine started with %d rules", len(self.rules))

    def stop(self) -> None:
        """Stop the correlation engine."""
        self._running = False
        logger.info("Correlation engine stopped")
