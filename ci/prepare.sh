#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(dirname "$(realpath "$0")")"
PROJECT_DIR=$(git -C "${SCRIPT_DIR}" rev-parse --show-toplevel)

echo "=== Stage: Prepare ==="
# A clone instead of a copy: only the committed state, without local build
# trees. `origin/*` of the clone are the local branches of the repository.
echo "Cloning ${PROJECT_DIR} to ${WORKSPACE}..."
rm -rf "${WORKSPACE}"
git clone --quiet "${PROJECT_DIR}" "${WORKSPACE}"
