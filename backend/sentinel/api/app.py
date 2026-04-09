"""FastAPI application factory."""

from contextlib import asynccontextmanager
from typing import AsyncGenerator

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from sentinel.api.routes import search, incidents, alerts, assets, threat_intel, dashboards
from sentinel.api.middleware.auth import AuthMiddleware


@asynccontextmanager
async def lifespan(app: FastAPI) -> AsyncGenerator[None, None]:
    """Application lifespan: startup and shutdown."""
    # Startup
    # TODO: Initialize gRPC connection pool to engine
    # TODO: Initialize database connection pool
    # TODO: Start Celery workers
    yield
    # Shutdown
    # TODO: Close connections gracefully


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
        allow_origins=["http://localhost:5173", "http://localhost:3000"],
        allow_credentials=True,
        allow_methods=["*"],
        allow_headers=["*"],
    )

    # Routes
    app.include_router(search.router, prefix="/api/search", tags=["search"])
    app.include_router(incidents.router, prefix="/api/incidents", tags=["incidents"])
    app.include_router(alerts.router, prefix="/api/alerts", tags=["alerts"])
    app.include_router(assets.router, prefix="/api/assets", tags=["assets"])
    app.include_router(threat_intel.router, prefix="/api/threat-intel", tags=["threat-intel"])
    app.include_router(dashboards.router, prefix="/api/dashboards", tags=["dashboards"])

    @app.get("/api/health")
    async def health_check():
        return {"status": "ok", "version": "0.1.0"}

    return app


app = create_app()
