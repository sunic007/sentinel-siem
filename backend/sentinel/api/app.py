"""FastAPI application factory."""

import asyncio
import logging
import os
from contextlib import asynccontextmanager
from typing import AsyncGenerator

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from sentinel.api.routes import (
    search, incidents, alerts, assets, threat_intel, dashboards, ingest, notables,
)
from sentinel.correlation.engine import run_correlation_loop
from sentinel.correlation.store import NotableStore
from sentinel.engine_client.client import EngineClient, EngineConfig

logger = logging.getLogger(__name__)


@asynccontextmanager
async def lifespan(app: FastAPI) -> AsyncGenerator[None, None]:
    """Application lifespan: startup → serve → shutdown."""

    # ── Engine client ────────────────────────────────────────────────────────
    engine_config = EngineConfig(
        host=os.getenv("ENGINE_HOST", "localhost"),
        port=int(os.getenv("ENGINE_PORT", "8080")),
    )
    engine_client = EngineClient(engine_config)
    await engine_client.connect()
    app.state.engine = engine_client

    # ── Notable event store ──────────────────────────────────────────────────
    notable_store = NotableStore()
    app.state.notable_store = notable_store

    # ── Correlation scheduler (background asyncio task) ──────────────────────
    correlation_interval = int(os.getenv("CORRELATION_INTERVAL", "60"))
    correlation_task = asyncio.create_task(
        run_correlation_loop(engine_client, notable_store, correlation_interval)
    )
    logger.info(
        "Sentinel SIEM backend started — correlation every %ds", correlation_interval
    )

    yield

    # ── Shutdown ─────────────────────────────────────────────────────────────
    correlation_task.cancel()
    try:
        await correlation_task
    except asyncio.CancelledError:
        pass

    await engine_client.disconnect()
    logger.info("Sentinel SIEM backend stopped")


def create_app() -> FastAPI:
    """Create and configure the FastAPI application."""
    app = FastAPI(
        title="Sentinel SIEM",
        description="Security Information and Event Management Platform",
        version="0.1.0",
        lifespan=lifespan,
    )

    # CORS
    app.add_middleware(
        CORSMiddleware,
        allow_origins=["*"],
        allow_credentials=True,
        allow_methods=["*"],
        allow_headers=["*"],
    )

    # Routes
    app.include_router(search.router,      prefix="/api/search",      tags=["search"])
    app.include_router(ingest.router,      prefix="/api/ingest",      tags=["ingest"])
    app.include_router(notables.router,    prefix="/api/notables",    tags=["notables"])
    app.include_router(incidents.router,   prefix="/api/incidents",   tags=["incidents"])
    app.include_router(alerts.router,      prefix="/api/alerts",      tags=["alerts"])
    app.include_router(assets.router,      prefix="/api/assets",      tags=["assets"])
    app.include_router(threat_intel.router,prefix="/api/threat-intel",tags=["threat-intel"])
    app.include_router(dashboards.router,  prefix="/api/dashboards",  tags=["dashboards"])

    @app.get("/api/health")
    async def health_check():
        engine = app.state.engine
        engine_health = await engine.health_check()
        notable_store: NotableStore = app.state.notable_store
        stats = {
            "total": await notable_store.count(),
            "open": await notable_store.count_open(),
            "by_severity": await notable_store.count_by_severity(),
        }
        return {
            "status": "ok",
            "version": "0.1.0",
            "engine": engine_health,
            "notables": stats,
        }

    @app.get("/api/indexes")
    async def list_indexes():
        engine = app.state.engine
        return await engine.list_indexes()

    return app


app = create_app()
