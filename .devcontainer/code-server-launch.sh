#!/usr/bin/env bash
set -euo pipefail

# Load Emscripten environment if available
if [ -f /etc/profile.d/emsdk.sh ]; then
    # shellcheck source=/etc/profile.d/emsdk.sh
    source /etc/profile.d/emsdk.sh
fi

CODE_SERVER_BIN=$(command -v code-server || true)
if [ -z "$CODE_SERVER_BIN" ]; then
    echo "Error: code-server binary not found. Run this script inside the Hyperion IDE container." >&2
    exit 1
fi

EXTENSIONS=${IDE_EXTENSIONS:-}
if [ -n "$EXTENSIONS" ]; then
    INSTALLED_EXTENSIONS=$("$CODE_SERVER_BIN" --list-extensions 2>/dev/null || true)
    for extension in $EXTENSIONS; do
        if ! grep -q "^${extension}$" <<<"$INSTALLED_EXTENSIONS"; then
            if "$CODE_SERVER_BIN" --install-extension "$extension" >/dev/null 2>&1; then
                echo "Installed extension: $extension"
            else
                echo "Warning: failed to install extension $extension" >&2
            fi
        fi
    done
fi

CERT_ARGS=("--cert=false")
if [ -n "${CERT_FILE:-}" ] && [ -n "${KEY_FILE:-}" ]; then
    CERT_ARGS=("--cert" "$CERT_FILE" "--cert-key" "$KEY_FILE")
fi

CMD=("$CODE_SERVER_BIN" --bind-addr 0.0.0.0:8443 --auth password)
CMD+=("${CERT_ARGS[@]}")
CMD+=(/home/coder/project)

exec "${CMD[@]}"
