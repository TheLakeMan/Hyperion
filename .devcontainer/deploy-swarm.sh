#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ENV_FILE="${SCRIPT_DIR}/.env"
STACK_FILE="${SCRIPT_DIR}/stack.yaml"

if [ ! -f "$ENV_FILE" ]; then
    echo "Error: ${ENV_FILE} not found." >&2
    exit 1
fi

if [ ! -f "$STACK_FILE" ]; then
    echo "Error: ${STACK_FILE} not found." >&2
    exit 1
fi

set -a
# shellcheck disable=SC1090
source "$ENV_FILE"
set +a

: "${IDE_IMAGE:?IDE_IMAGE must be set in .env}"

echo "Building image ${IDE_IMAGE}..."
docker build -t "$IDE_IMAGE" "$SCRIPT_DIR"

echo "Deploying Hyperion IDE stack to Docker Swarm..."
docker stack deploy --compose-file "$STACK_FILE" hyperion-ide

echo "Stack deployed. Use 'docker stack services hyperion-ide' to check status."
