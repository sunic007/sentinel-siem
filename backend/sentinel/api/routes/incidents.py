"""Incident management API routes."""

from datetime import datetime
from enum import Enum
from typing import Optional

from fastapi import APIRouter, HTTPException
from pydantic import BaseModel, Field

router = APIRouter()


class Severity(str, Enum):
    INFO = "info"
    LOW = "low"
    MEDIUM = "medium"
    HIGH = "high"
    CRITICAL = "critical"


class IncidentStatus(str, Enum):
    NEW = "new"
    IN_PROGRESS = "in_progress"
    RESOLVED = "resolved"
    CLOSED = "closed"


class Incident(BaseModel):
    id: Optional[str] = None
    title: str
    description: str = ""
    severity: Severity = Severity.MEDIUM
    status: IncidentStatus = IncidentStatus.NEW
    assignee: Optional[str] = None
    rule_id: Optional[str] = None
    contributing_events: list[dict] = []
    created_at: Optional[datetime] = None
    updated_at: Optional[datetime] = None


class IncidentUpdate(BaseModel):
    status: Optional[IncidentStatus] = None
    severity: Optional[Severity] = None
    assignee: Optional[str] = None
    comment: Optional[str] = None


@router.get("/", response_model=list[Incident])
async def list_incidents(
    status: Optional[IncidentStatus] = None,
    severity: Optional[Severity] = None,
    limit: int = 50,
    offset: int = 0,
):
    """List incidents with optional filtering."""
    # TODO: Query PostgreSQL
    return []


@router.post("/", response_model=Incident)
async def create_incident(incident: Incident):
    """Create a new incident."""
    # TODO: Insert into PostgreSQL
    incident.id = "INC-001"
    incident.created_at = datetime.utcnow()
    return incident


@router.get("/{incident_id}", response_model=Incident)
async def get_incident(incident_id: str):
    """Get incident details."""
    raise HTTPException(status_code=404, detail="Incident not found")


@router.patch("/{incident_id}", response_model=Incident)
async def update_incident(incident_id: str, update: IncidentUpdate):
    """Update incident status, severity, or assignment."""
    raise HTTPException(status_code=404, detail="Incident not found")


@router.get("/{incident_id}/events")
async def get_incident_events(incident_id: str):
    """Get contributing events for an incident."""
    return []


@router.post("/{incident_id}/comments")
async def add_comment(incident_id: str, comment: dict):
    """Add a comment to an incident."""
    return {"status": "ok"}
