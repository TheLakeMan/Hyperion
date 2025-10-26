#!/usr/bin/env bash
set -euo pipefail

echo "Removing Hyperion IDE stack from Docker Swarm..."
docker stack rm hyperion-ide

echo "Waiting for services to terminate..."
while docker stack services hyperion-ide >/dev/null 2>&1; do
    sleep 1
done

echo "Stack removed."
