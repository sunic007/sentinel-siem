"""
Correlation engine — evaluates built-in (and custom) rules against the search
engine and creates notable events when rules fire.

Design
------
* Rules are defined as SPL queries (see rules.py).
* The engine fires each rule's query against the C++ search engine via the
  existing EngineClient.
* When a query returns results that satisfy the rule's condition the engine
  creates a Notable and persists it in the in-memory NotableStore.
* The scheduler (called from FastAPI lifespan) invokes run_all() periodically.
"""

import asyncio
import logging
import uuid
from datetime import datetime, timezone
from typing import Any

from sentinel.correlation.rules import BUILTIN_RULES, BuiltinRule
from sentinel.correlation.store import Notable, NotableStore

logger = logging.getLogger(__name__)


# ---------------------------------------------------------------------------
# Condition evaluator
# ---------------------------------------------------------------------------

def _condition_met(rule: BuiltinRule, result: dict[str, Any]) -> bool:
    """Return True if search result satisfies the rule's firing condition."""
    total = result.get("total_count", 0)
    events = result.get("events", [])

    cond = rule.condition

    if "count_gt" in cond:
        # total_count > N  OR  any aggregated row with count > N
        threshold = cond["count_gt"]
        if total > threshold:
            return True
        # aggregated results: each event has a "count" field
        for ev in events:
            try:
                if int(ev.get("count", 0)) > threshold:
                    return True
            except (ValueError, TypeError):
                pass
        return False

    if "count_ge" in cond:
        threshold = cond["count_ge"]
        if total >= threshold:
            return True
        for ev in events:
            try:
                if int(ev.get("count", 0)) >= threshold:
                    return True
            except (ValueError, TypeError):
                pass
        return False

    # Default: any result → fire
    return total > 0 or len(events) > 0


# ---------------------------------------------------------------------------
# Correlation engine
# ---------------------------------------------------------------------------

class CorrelationEngine:
    """Evaluates correlation rules against the search engine."""

    def __init__(self, engine_client: Any, store: NotableStore) -> None:
        self._client = engine_client
        self._store = store
        self._custom_rules: list[BuiltinRule] = []

    def add_custom_rule(self, rule: BuiltinRule) -> None:
        self._custom_rules.append(rule)

    async def run_all(self) -> int:
        """Run every enabled rule.  Returns count of new notables created."""
        all_rules = BUILTIN_RULES + self._custom_rules
        created = 0

        tasks = [self._evaluate_rule(rule) for rule in all_rules]
        results = await asyncio.gather(*tasks, return_exceptions=True)

        for rule, result in zip(all_rules, results):
            if isinstance(result, Exception):
                logger.warning("Rule %s evaluation error: %s", rule.id, result)
                continue
            if result:
                await self._store.add(result)
                created += 1
                logger.info(
                    "Notable created: [%s] %s (rule=%s)",
                    result.severity.upper(),
                    result.title,
                    rule.id,
                )

        if created:
            logger.info("Correlation run complete — %d new notables", created)
        return created

    async def _evaluate_rule(self, rule: BuiltinRule) -> Notable | None:
        """Run a single rule.  Returns a Notable if it fires, else None."""
        try:
            result = await self._client.execute_query(rule.query)
            result_dict = {
                "total_count": result.total_count,
                "events": result.events,
            }
        except Exception as exc:
            logger.debug("Rule %s query failed: %s", rule.id, exc)
            return None

        if not _condition_met(rule, result_dict):
            return None

        # Extract contributing hosts from result events
        hosts = list({
            ev.get("host", "")
            for ev in result.events[:10]
            if ev.get("host")
        })

        return Notable(
            id=str(uuid.uuid4()),
            rule_id=rule.id,
            rule_name=rule.name,
            severity=rule.severity,
            title=rule.name,
            description=rule.description,
            mitre=rule.mitre,
            risk_score=rule.risk_score,
            status="new",
            host=", ".join(hosts[:3]) if hosts else "",
            source="correlation",
            contributing_events=result.events[:5],
            created_at=datetime.now(timezone.utc),
            updated_at=datetime.now(timezone.utc),
        )


# ---------------------------------------------------------------------------
# Background scheduler
# ---------------------------------------------------------------------------

async def run_correlation_loop(
    engine_client: Any,
    store: NotableStore,
    interval_seconds: int = 60,
) -> None:
    """
    Infinite asyncio loop that runs all correlation rules every `interval_seconds`.
    Designed to run as a background task from FastAPI lifespan.
    """
    corr = CorrelationEngine(engine_client, store)
    logger.info(
        "Correlation scheduler started (interval=%ds, rules=%d)",
        interval_seconds,
        len(BUILTIN_RULES),
    )

    # Run immediately on startup (don't wait for first interval)
    await asyncio.sleep(5)   # brief delay so engine client is fully ready
    try:
        await corr.run_all()
    except Exception as exc:
        logger.warning("Initial correlation run error: %s", exc)

    while True:
        await asyncio.sleep(interval_seconds)
        try:
            await corr.run_all()
        except asyncio.CancelledError:
            break
        except Exception as exc:
            logger.warning("Correlation run error: %s", exc)

    logger.info("Correlation scheduler stopped")
