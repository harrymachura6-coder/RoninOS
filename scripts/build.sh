#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE_TAG="roninos-buildenv:local"

cd "$ROOT_DIR"

docker build -f buildenv/Dockerfile -t "$IMAGE_TAG" .
docker run --rm \
  -v "$ROOT_DIR":/workspace \
  -w /workspace \
  "$IMAGE_TAG" \
  bash -lc 'make clean && make'
