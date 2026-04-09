#!/bin/bash
set -e

echo "=== Sentinel SIEM - Development Setup ==="

# Check prerequisites
echo "Checking prerequisites..."

check_command() {
    if ! command -v "$1" &> /dev/null; then
        echo "ERROR: $1 is not installed. Please install it first."
        exit 1
    fi
    echo "  ✓ $1 found"
}

check_command cmake
check_command python3
check_command node
check_command npm

# C++ Engine
echo ""
echo "=== Setting up C++ Engine ==="
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DSENTINEL_ENABLE_SANITIZERS=ON
cmake --build build --parallel $(nproc 2>/dev/null || echo 4)

# Python Backend
echo ""
echo "=== Setting up Python Backend ==="
cd backend
if command -v poetry &> /dev/null; then
    poetry install
else
    echo "Poetry not found, using pip..."
    python3 -m venv .venv
    source .venv/bin/activate
    pip install -e ".[dev]"
fi
cd ..

# Frontend
echo ""
echo "=== Setting up Frontend ==="
cd frontend
npm install
cd ..

# Generate proto stubs
echo ""
echo "=== Generating Proto Stubs ==="
bash scripts/generate-proto.sh

echo ""
echo "=== Setup Complete ==="
echo "To start development:"
echo "  Engine:   cd build && ./engine/sentinel_engine_server"
echo "  Backend:  cd backend && uvicorn sentinel.api.app:app --reload"
echo "  Frontend: cd frontend && npm run dev"
echo "  Docker:   docker compose -f deploy/docker/docker-compose.yml up"
