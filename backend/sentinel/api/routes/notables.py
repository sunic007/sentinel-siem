"""Notable events API routes."""

from datetime import datetime
from typing import Optional

from fastapi import APIRouter, HTTPException, Request
from pydantic import BaseModel

router = APIRouter()


class NotableResponse(BaseModel):
    id: str
    rule_id: str
    rule_name: str
    severity: str
    title: str
    description: str
    mitre: list[str]
    risk_score: int
    status: str
    source: str
    host: str
    contributing_events: list[dict]
    created_at: datetime
    updated_at: datetime


class NotableStatusUpdate(BaseModel):
    status: str  # new | in_progress | resolved | closed


class NotableStats(BaseModel):
    total: int
    open: int
    by_severity: dict[str, int]


@router.get("/", response_model=list[NotableResponse])
async def list_notables(
    req: Request,
    severity: Optional[str] = None,
    status: Optional[str] = None,
    limit: int = 100,
    offset: int = 0,
):
    """List notable events with optional filtering."""
    store = req.app.state.notable_store
    notables = await store.list(
        severity=severity, status=status, limit=limit, offset=offset
    )
    return [
        NotableResponse(
            id=n.id,
            rule_id=n.rule_id,
            rule_name=n.rule_name,
            severity=n.severity,
            title=n.title,
            description=n.description,
            mitre=n.mitre,
            risk_score=n.risk_score,
            status=n.status,
            source=n.source,
            host=n.host,
            contributing_events=n.contributing_events,
            created_at=n.created_at,
            updated_at=n.updated_at,
        )
        for n in notables
    ]


@router.get("/stats", response_model=NotableStats)
async def notable_stats(req: Request):
    """Return summary counts for dashboard widgets."""
    store = req.app.state.notable_store
    total = await store.count()
    open_count = await store.count_open()
    by_severity = await store.count_by_severity()
    return NotableStats(total=total, open=open_count, by_severity=by_severity)


@router.get("/{notable_id}", response_model=NotableResponse)
async def get_notable(notable_id: str, req: Request):
    """Get a single notable event by ID."""
    store = req.app.state.notable_store
    n = await store.get(notable_id)
    if not n:
        raise HTTPException(status_code=404, detail="Notable not found")
    return NotableResponse(
        id=n.id,
        rule_id=n.rule_id,
        rule_name=n.rule_name,
        severity=n.severity,
        title=n.title,
        description=n.description,
        mitre=n.mitre,
        risk_score=n.risk_score,
        status=n.status,
        source=n.source,
        host=n.host,
        contributing_events=n.contributing_events,
        created_at=n.created_at,
        updated_at=n.updated_at,
    )


@router.patch("/{notable_id}/status", response_model=NotableResponse)
async def update_notable_status(
    notable_id: str, update: NotableStatusUpdate, req: Request
):
    """Update a notable event's triage status."""
    valid_statuses = {"new", "in_progress", "resolved", "closed"}
    if update.status not in valid_statuses:
        raise HTTPException(
            status_code=422,
            detail=f"status must be one of: {', '.join(sorted(valid_statuses))}",
        )

    store = req.app.state.notable_store
    n = await store.update_status(notable_id, update.status)
    if not n:
        raise HTTPException(status_code=404, detail="Notable not found")

    return NotableResponse(
        id=n.id,
        rule_id=n.rule_id,
        rule_name=n.rule_name,
        severity=n.severity,
        title=n.title,
        description=n.description,
        mitre=n.mitre,
        risk_score=n.risk_score,
        status=n.status,
        source=n.source,
        host=n.host,
        contributing_events=n.contributing_events,
        created_at=n.created_at,
        updated_at=n.updated_at,
    )
