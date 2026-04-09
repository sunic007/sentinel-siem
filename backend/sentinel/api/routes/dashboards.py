"""Dashboard management API routes."""

from typing import Optional

from fastapi import APIRouter
from pydantic import BaseModel

router = APIRouter()


class DashboardPanel(BaseModel):
    id: str
    title: str
    query: str  # SPL query
    visualization: str = "table"  # table, line, bar, pie, single_value, map
    position: dict = {"x": 0, "y": 0, "w": 6, "h": 4}
    options: dict = {}  # Visualization-specific options
    refresh_interval: int = 300  # seconds


class Dashboard(BaseModel):
    id: Optional[str] = None
    title: str
    description: str = ""
    panels: list[DashboardPanel] = []
    is_default: bool = False
    owner: Optional[str] = None


@router.get("/", response_model=list[Dashboard])
async def list_dashboards():
    """List all dashboards."""
    return [
        Dashboard(
            id="security-posture",
            title="Security Posture",
            description="Overview of security status",
            is_default=True,
            panels=[
                DashboardPanel(
                    id="events-over-time",
                    title="Events Over Time",
                    query="search index=* | timechart count",
                    visualization="line",
                    position={"x": 0, "y": 0, "w": 12, "h": 4},
                ),
                DashboardPanel(
                    id="notable-by-severity",
                    title="Notable Events by Severity",
                    query="search index=notable | stats count by severity",
                    visualization="pie",
                    position={"x": 0, "y": 4, "w": 6, "h": 4},
                ),
                DashboardPanel(
                    id="top-sources",
                    title="Top Event Sources",
                    query="search index=* | stats count by source | sort -count | head 10",
                    visualization="bar",
                    position={"x": 6, "y": 4, "w": 6, "h": 4},
                ),
            ],
        ),
    ]


@router.post("/", response_model=Dashboard)
async def create_dashboard(dashboard: Dashboard):
    """Create a new dashboard."""
    dashboard.id = "dash-new"
    return dashboard


@router.get("/{dashboard_id}", response_model=Dashboard)
async def get_dashboard(dashboard_id: str):
    """Get dashboard details with panels."""
    return Dashboard(title="Not Found")


@router.put("/{dashboard_id}", response_model=Dashboard)
async def update_dashboard(dashboard_id: str, dashboard: Dashboard):
    """Update a dashboard."""
    return dashboard


@router.delete("/{dashboard_id}")
async def delete_dashboard(dashboard_id: str):
    """Delete a dashboard."""
    return {"deleted": True}


@router.post("/{dashboard_id}/clone", response_model=Dashboard)
async def clone_dashboard(dashboard_id: str):
    """Clone a dashboard."""
    return Dashboard(title="Cloned Dashboard")
