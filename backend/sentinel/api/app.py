"""FastAPI application factory."""

import os
from contextlib import asynccontextmanager
from typing import AsyncGenerator

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from sentinel.api.routes import search, incidents, alerts, assets, threat_intel, dashboards, ingest
from sentinel.engine_client.client import EngineClient, EngineConfig


@asynccontextmanager
async def lifespan(app: FastAPI) -> AsyncGenerator[None, None]:
    """Application lifespan: startup and shutdown."""
    # Initialize engine client
    engine_config = EngineConfig(
        host=os.getenv("ENGINE_HOST", "localhost"),
        port=int(os.getenv("ENGINE_PORT", "8080")),
    )
    engine_client = EngineClient(engine_config)
    await engine_client.connect()
    app.state.engine = engine_client

    yield

    # Shutdown
    await engine_client.disconnect()


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
    app.include_router(search.router, prefix="/api/search", tags=["search"])
    app.include_router(ingest.router, prefix="/api/ingest", tags=["ingest"])
    app.include_router(incidents.router, prefix="/api/incidents", tags=["incidents"])
    app.include_router(alerts.router, prefix="/api/alerts", tags=["alerts"])
    app.include_router(assets.router, prefix="/api/assets", tags=["assets"])
    app.include_router(threat_intel.router, prefix="/api/threat-intel", tags=["threat-intel"])
    app.include_router(dashboards.router, prefix="/api/dashboards", tags=["dashboards"])

    @app.get("/api/health")
    async def health_check():
        engine = app.state.engine
        engine_health = await engine.health_check()
        return {
            "status": "ok",
            "version": "0.1.0",
            "engine": engine_health,
        }

    @app.get("/api/indexes")
    async def list_indexes():
        engine = app.state.engine
        return await engine.list_indexes()

    return app


app = create_app()
