"""Search API routes — execute SPL queries against the engine."""

from datetime import datetime
from typing import Optional

from fastapi import APIRouter, HTTPException
from pydantic import BaseModel, Field

router = APIRouter()


class SearchRequest(BaseModel):
    query: str = Field(..., description="SPL query string")
    earliest: Optional[datetime] = None
    latest: Optional[datetime] = None
    max_results: int = Field(default=10000, ge=0, le=100000)


class SearchEvent(BaseModel):
    time: str
    raw: str
    host: str
    source: str
    sourcetype: str
    fields: dict[str, str] = {}


class SearchResponse(BaseModel):
    events: list[SearchEvent] = []
    stats: Optional[dict] = None
    total_count: int = 0
    execution_time_ms: float = 0
    columns: list[str] = []


class ParseResponse(BaseModel):
    valid: bool
    errors: list[str] = []
    referenced_fields: list[str] = []
    referenced_indexes: list[str] = []


@router.post("/execute", response_model=SearchResponse)
async def execute_search(request: SearchRequest):
    """Execute an SPL query and return results."""
    # TODO: Forward to C++ engine via gRPC
    return SearchResponse(
        events=[],
        total_count=0,
        execution_time_ms=0,
        columns=[],
    )


@router.post("/parse", response_model=ParseResponse)
async def parse_query(request: SearchRequest):
    """Parse an SPL query and validate syntax."""
    # TODO: Forward to C++ engine via gRPC
    return ParseResponse(valid=True)


@router.get("/jobs/{search_id}")
async def get_search_status(search_id: str):
    """Get the status of a running search job."""
    return {"search_id": search_id, "state": "completed"}


@router.delete("/jobs/{search_id}")
async def cancel_search(search_id: str):
    """Cancel a running search job."""
    return {"search_id": search_id, "cancelled": True}
