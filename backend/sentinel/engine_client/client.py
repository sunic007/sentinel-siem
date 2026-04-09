"""HTTP client for communicating with the C++ engine."""

import logging
from dataclasses import dataclass
from typing import Optional

import httpx

logger = logging.getLogger(__name__)


@dataclass
class EngineConfig:
    host: str = "localhost"
    port: int = 8080
    timeout_seconds: float = 30.0

    @property
    def base_url(self) -> str:
        return f"http://{self.host}:{self.port}"


@dataclass
class SearchResult:
    events: list[dict]
    stats: Optional[dict]
    total_count: int
    execution_time_ms: float
    columns: list[str]


class EngineClient:
    """HTTP client for the Sentinel C++ engine."""

    def __init__(self, config: EngineConfig | None = None):
        self.config = config or EngineConfig()
        self._client: Optional[httpx.AsyncClient] = None

    async def connect(self) -> None:
        """Create the HTTP client."""
        self._client = httpx.AsyncClient(
            base_url=self.config.base_url,
            timeout=self.config.timeout_seconds,
        )
        logger.info("Engine client connected to %s", self.config.base_url)

    async def disconnect(self) -> None:
        """Close the HTTP client."""
        if self._client:
            await self._client.aclose()
            self._client = None

    async def execute_query(self, query: str, **kwargs) -> SearchResult:
        """Execute an SPL query against the engine."""
        if not self._client:
            await self.connect()

        try:
            resp = await self._client.post("/api/search", json={"query": query, **kwargs})
            resp.raise_for_status()
            data = resp.json()

            return SearchResult(
                events=data.get("events", []),
                stats=data.get("stats"),
                total_count=data.get("total_count", 0),
                execution_time_ms=data.get("execution_time_ms", 0),
                columns=data.get("columns", []),
            )
        except httpx.ConnectError:
            logger.warning("Engine not reachable at %s", self.config.base_url)
            return SearchResult(events=[], stats=None, total_count=0, execution_time_ms=0, columns=[])
        except Exception as e:
            logger.error("Search query failed: %s", e)
            return SearchResult(events=[], stats=None, total_count=0, execution_time_ms=0, columns=[])

    async def ingest_events(
        self,
        events: list[dict],
        index: str = "main",
        sourcetype: str = "json",
        host: str = "unknown",
        source: str = "http",
    ) -> dict:
        """Send events to the engine for indexing."""
        if not self._client:
            await self.connect()

        try:
            resp = await self._client.post("/api/ingest", json={
                "index": index,
                "sourcetype": sourcetype,
                "host": host,
                "source": source,
                "events": events,
            })
            resp.raise_for_status()
            return resp.json()
        except httpx.ConnectError:
            logger.warning("Engine not reachable for ingest")
            return {"accepted": 0, "error": "engine not reachable"}
        except Exception as e:
            logger.error("Ingest failed: %s", e)
            return {"accepted": 0, "error": str(e)}

    async def list_indexes(self) -> list[dict]:
        """List all indexes."""
        if not self._client:
            await self.connect()

        try:
            resp = await self._client.get("/api/indexes")
            resp.raise_for_status()
            return resp.json()
        except Exception as e:
            logger.error("List indexes failed: %s", e)
            return []

    async def health_check(self) -> dict:
        """Check engine health."""
        if not self._client:
            await self.connect()

        try:
            resp = await self._client.get("/api/health")
            resp.raise_for_status()
            return resp.json()
        except httpx.ConnectError:
            return {"status": "unreachable"}
        except Exception as e:
            logger.error("Health check failed: %s", e)
            return {"status": "error", "detail": str(e)}
