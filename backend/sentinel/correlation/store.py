"""
In-memory notable event store.

Thread-safe with asyncio.Lock.  Notable events are keyed by a UUID.
The store caps at MAX_NOTABLES entries (FIFO eviction).
"""

import asyncio
import uuid
from dataclasses import dataclass, field
from datetime import datetime, timezone
from typing import Optional


MAX_NOTABLES = 10_000


@dataclass
class Notable:
    id: str
    rule_id: str
    rule_name: str
    severity: str
    title: str
    description: str
    mitre: list[str]
    risk_score: int
    status: str = "new"           # new | in_progress | resolved | closed
    source: str = ""              # index / sourcetype hint
    host: str = ""
    contributing_events: list[dict] = field(default_factory=list)
    created_at: datetime = field(default_factory=lambda: datetime.now(timezone.utc))
    updated_at: datetime = field(default_factory=lambda: datetime.now(timezone.utc))


class NotableStore:
    """Singleton in-memory store for notable events."""

    def __init__(self) -> None:
        self._lock = asyncio.Lock()
        self._notables: dict[str, Notable] = {}   # id → Notable
        self._order: list[str] = []               # insertion order

    async def add(self, notable: Notable) -> None:
        async with self._lock:
            self._notables[notable.id] = notable
            self._order.append(notable.id)
            # Evict oldest when over cap
            while len(self._order) > MAX_NOTABLES:
                evict_id = self._order.pop(0)
                self._notables.pop(evict_id, None)

    async def get(self, notable_id: str) -> Optional[Notable]:
        async with self._lock:
            return self._notables.get(notable_id)

    async def update_status(self, notable_id: str, status: str) -> Optional[Notable]:
        async with self._lock:
            n = self._notables.get(notable_id)
            if n:
                n.status = status
                n.updated_at = datetime.now(timezone.utc)
            return n

    async def list(
        self,
        severity: Optional[str] = None,
        status: Optional[str] = None,
        limit: int = 100,
        offset: int = 0,
    ) -> list[Notable]:
        async with self._lock:
            items = list(reversed([self._notables[i] for i in self._order
                                    if i in self._notables]))
            if severity:
                items = [n for n in items if n.severity == severity]
            if status:
                items = [n for n in items if n.status == status]
            return items[offset: offset + limit]

    async def count(self) -> int:
        async with self._lock:
            return len(self._notables)

    async def count_by_severity(self) -> dict[str, int]:
        async with self._lock:
            counts: dict[str, int] = {}
            for n in self._notables.values():
                counts[n.severity] = counts.get(n.severity, 0) + 1
            return counts

    async def count_open(self) -> int:
        """Count notables in 'new' or 'in_progress' status."""
        async with self._lock:
            return sum(1 for n in self._notables.values()
                       if n.status in ("new", "in_progress"))


# Global singleton — initialised in lifespan
_store: Optional["NotableStore"] = None


def get_store() -> "NotableStore":
    global _store
    if _store is None:
        _store = NotableStore()
    return _store
