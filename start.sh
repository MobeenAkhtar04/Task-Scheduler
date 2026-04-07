#!/bin/bash
set -e

ROOT="$(cd "$(dirname "$0")" && pwd)"

echo "Starting scheduler..."
cd "$ROOT/docker"
docker compose up --build &
COMPOSE_PID=$!

echo "Waiting for WebSocket server on port 8765..."
until nc -z localhost 8765 2>/dev/null; do
  sleep 1
done

echo "Opening dashboard..."
open "$ROOT/dashboard/index.html"

echo "Dashboard is live. Press Ctrl+C to stop."
trap "docker compose down; exit 0" INT TERM
wait $COMPOSE_PID
