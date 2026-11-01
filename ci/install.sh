#!/usr/bin/env bash

set -euo pipefail
source "$(dirname "$(realpath "$0")")/common.sh"

echo "=== Stage: Install ==="
cmake --install "${BUILD_DIR}/libnova" --config "${BUILD_TYPE}" --prefix "${INSTALL_DIR}"
