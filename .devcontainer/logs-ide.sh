#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ENV_FILE="${SCRIPT_DIR}/.env"
COMPOSE_FILE="${SCRIPT_DIR}/compose.yaml"

if command -v docker-compose >/dev/null 2>&1; then
    COMPOSE_BIN=(docker-compose)
    SUPPORTS_ENV_FILE=false
elif docker compose version >/dev/null 2>&1; then
    COMPOSE_BIN=(docker compose)
    SUPPORTS_ENV_FILE=true
else
    echo "Error: docker compose is not available." >&2
    exit 1
fi

cd "$SCRIPT_DIR"

if [ "$SUPPORTS_ENV_FILE" = true ] && [ -f "$ENV_FILE" ]; then
    ${COMPOSE_BIN[@]} --env-file "$ENV_FILE" -f "$COMPOSE_FILE" logs -f hyperion-ide
else
    ${COMPOSE_BIN[@]} -f "$COMPOSE_FILE" logs -f hyperion-ide
fi
