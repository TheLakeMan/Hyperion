#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ENV_FILE="${SCRIPT_DIR}/.env"
COMPOSE_FILE="${SCRIPT_DIR}/compose.yaml"

if [ ! -f "$ENV_FILE" ]; then
    echo "Error: ${ENV_FILE} not found. Create it before starting the IDE." >&2
    exit 1
fi

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

set -a
# shellcheck disable=SC1090
source "$ENV_FILE"
set +a

if [ -z "${IDE_PASSWORD:-}" ] || [ "$IDE_PASSWORD" = "changeme" ]; then
    echo "Warning: IDE_PASSWORD is unset or still 'changeme'. Update ${ENV_FILE} for better security." >&2
fi

cd "$SCRIPT_DIR"

if [ "${SUPPORTS_ENV_FILE:-false}" = true ]; then
    ${COMPOSE_BIN[@]} --env-file "$ENV_FILE" -f "$COMPOSE_FILE" up -d --build
else
    ${COMPOSE_BIN[@]} -f "$COMPOSE_FILE" up -d --build
fi

echo "Hyperion IDE is starting. Access it at http://localhost:${IDE_PORT:-8443}"
