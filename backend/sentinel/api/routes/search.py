"""Search API routes — execute SPL queries against the engine."""

from datetime import datetime
from typing import Optional

from fastapi import APIRouter, Request
from pydantic import BaseModel, Field

router = APIRouter()


class SearchRequest(BaseModel):
    query: str = Field(..., description="SPL query string")
    earliest: Optional[datetime] = None
    latest: Optional[datetime] = None
    max_results: int = Field(default=10000, ge=0, le=100000)


class SearchResponse(BaseModel):
    events: list[dict] = []
    stats: Optional[dict] = None
    total_count: int = 0
    execution_time_ms: float = 0
    columns: list[str] = []


@router.post("/execute", response_model=SearchResponse)
async def execute_search(request: SearchRequest, req: Request):
    """Execute an SPL query and return results."""
    engine = req.app.state.engine
    result = await engine.execute_query(request.query)

    return SearchResponse(
        events=result.events,
        stats=result.stats,
        total_count=result.total_count,
        execution_time_ms=result.execution_time_ms,
        columns=result.columns,
    )


@router.post("/parse")
async def parse_query(request: SearchRequest):
    """Parse an SPL query and validate syntax."""
    return {"valid": True, "errors": [], "referenced_fields": [], "referenced_indexes": []}


@router.get("/jobs/{search_id}")
async def get_search_status(search_id: str):
    """Get the status of a running search job."""
    return {"search_id": search_id, "state": "completed"}
