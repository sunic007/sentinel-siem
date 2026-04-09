"""Threat Intelligence API routes."""

from datetime import datetime
from typing import Optional

from fastapi import APIRouter
from pydantic import BaseModel

router = APIRouter()


class ThreatFeed(BaseModel):
    id: Optional[str] = None
    name: str
    url: str
    feed_type: str = "stix"  # stix, csv, json
    enabled: bool = True
    poll_interval: int = 3600  # seconds
    last_poll: Optional[datetime] = None
    ioc_count: int = 0


class IOC(BaseModel):
    id: Optional[str] = None
    type: str  # ip, domain, hash, url, email
    value: str
    source: str
    confidence: float = 0.5
    severity: str = "medium"
    tags: list[str] = []
    first_seen: Optional[datetime] = None
    last_seen: Optional[datetime] = None
    tlp: str = "white"  # white, green, amber, red


@router.get("/feeds", response_model=list[ThreatFeed])
async def list_feeds():
    """List configured threat intelligence feeds."""
    return []


@router.post("/feeds", response_model=ThreatFeed)
async def add_feed(feed: ThreatFeed):
    """Add a new threat intelligence feed."""
    return feed


@router.delete("/feeds/{feed_id}")
async def delete_feed(feed_id: str):
    """Remove a threat intelligence feed."""
    return {"deleted": True}


@router.post("/feeds/{feed_id}/poll")
async def poll_feed(feed_id: str):
    """Manually trigger a feed poll."""
    return {"status": "polling"}


@router.get("/iocs", response_model=list[IOC])
async def search_iocs(
    type: Optional[str] = None,
    value: Optional[str] = None,
    source: Optional[str] = None,
    limit: int = 100,
):
    """Search IOCs."""
    return []


@router.post("/iocs", response_model=IOC)
async def add_ioc(ioc: IOC):
    """Add a manual IOC."""
    return ioc


@router.get("/iocs/matches")
async def get_ioc_matches(limit: int = 50):
    """Get recent IOC matches against ingested data."""
    return []
