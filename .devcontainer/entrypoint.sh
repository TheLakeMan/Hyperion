#!/usr/bin/env bash
set -euo pipefail

if [ "$(id -u)" -ne 0 ]; then
    exec /usr/local/bin/code-server-launch.sh
fi

mkdir -p /home/coder/.local/share/code-server/User
chown -R coder:coder /home/coder/.local/share/code-server

export HOME=/home/coder

exec su -m -s /bin/bash coder -c /usr/local/bin/code-server-launch.sh
