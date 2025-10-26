#!/usr/bin/env bash
set -euo pipefail

TARGET_DIR=${ACTIVEPIECES_DIR:-/home/thelakeman/tools/activepieces}
REPO_URL=https://github.com/activepieces/activepieces.git

if ! command -v git >/dev/null 2>&1; then
    echo "Error: git command not found." >&2
    exit 1
fi

mkdir -p "$(dirname "$TARGET_DIR")"

if [ -d "$TARGET_DIR/.git" ]; then
    echo "Updating existing Activepieces repository at $TARGET_DIR"
    git -C "$TARGET_DIR" fetch --all --prune
    git -C "$TARGET_DIR" pull --ff-only
else
    echo "Cloning Activepieces repository into $TARGET_DIR"
    git clone "$REPO_URL" "$TARGET_DIR"
fi

echo "Activepieces repository is ready at $TARGET_DIR"
