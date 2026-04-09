"""gRPC client for communicating with the C++ engine."""

import logging
from dataclasses import dataclass
from typing import AsyncIterator, Optional

logger = logging.getLogger(__name__)


@dataclass
class EngineConfig:
    host: str = "localhost"
    port: int = 50051
    max_channels: int = 4
    timeout_seconds: float = 30.0


@dataclass
class SearchResult:
    events: list[dict]
    stats: Optional[dict]
    total_count: int
    execution_time_ms: float


class EngineClient:
    """gRPC client for the Sentinel C++ engine."""

    def __init__(self, config: EngineConfig | None = None):
        self.config = config or EngineConfig()
        self._channel = None

    async def connect(self) -> None:
        """Establish gRPC connection to the engine."""
        # TODO: Create gRPC channel using generated stubs
        logger.info("Connecting to engine at %s:%d", self.config.host, self.config.port)

    async def disconnect(self) -> None:
        """Close the gRPC connection."""
        if self._channel:
            # TODO: Close gRPC channel
            pass

    async def execute_query(self, query: str, **kwargs) -> SearchResult:
        """Execute an SPL query against the engine."""
        # TODO: Use generated gRPC stubs to call SearchService.ExecuteQuerySync
        logger.info("Executing query: %s", query[:100])
        return SearchResult(events=[], stats=None, total_count=0, execution_time_ms=0)

    async def execute_query_stream(self, query: str, **kwargs) -> AsyncIterator[dict]:
        """Execute an SPL query and stream results."""
        # TODO: Use SearchService.ExecuteQuery (streaming)
        yield {}

    async def ingest_events(self, events: list[dict], index: str = "main") -> int:
        """Send events to the engine for indexing."""
        # TODO: Use IngestService.SendEvents
        return len(events)

    async def list_indexes(self) -> list[dict]:
        """List all indexes."""
        # TODO: Use IndexerService.ListIndexes
        return []

    async def health_check(self) -> dict:
        """Check engine health."""
        # TODO: Use IndexerService.HealthCheck
        return {"status": "unknown"}
