#!/bin/bash
set -e

PROTO_DIR="proto"
PYTHON_OUT="backend/sentinel/engine_client/generated"

echo "Generating Python gRPC stubs..."
mkdir -p "$PYTHON_OUT"

python3 -m grpc_tools.protoc \
    -I"$PROTO_DIR" \
    --python_out="$PYTHON_OUT" \
    --grpc_python_out="$PYTHON_OUT" \
    --pyi_out="$PYTHON_OUT" \
    "$PROTO_DIR"/*.proto

# Create __init__.py
touch "$PYTHON_OUT/__init__.py"

echo "Proto stubs generated in $PYTHON_OUT"
