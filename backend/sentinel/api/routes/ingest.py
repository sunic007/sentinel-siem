"""Event ingestion API routes."""

from fastapi import APIRouter, Request
from pydantic import BaseModel, Field

router = APIRouter()


class IngestEvent(BaseModel):
    raw: str
    host: str = "unknown"
    source: str = "http"
    sourcetype: str = "json"
    timestamp_us: int = 0


class IngestRequest(BaseModel):
    index: str = "main"
    sourcetype: str = "json"
    host: str = "unknown"
    source: str = "http"
    events: list[IngestEvent] = Field(..., min_length=1)


class IngestResponse(BaseModel):
    accepted: int = 0
    rejected: int = 0
    total_events: int = 0


@router.post("/", response_model=IngestResponse)
async def ingest_events(request: IngestRequest, req: Request):
    """Ingest a batch of events into the engine."""
    engine = req.app.state.engine
    events = [e.model_dump() for e in request.events]

    result = await engine.ingest_events(
        events=events,
        index=request.index,
        sourcetype=request.sourcetype,
        host=request.host,
        source=request.source,
    )

    return IngestResponse(
        accepted=result.get("accepted", 0),
        rejected=result.get("rejected", 0),
        total_events=result.get("total_events", 0),
    )
