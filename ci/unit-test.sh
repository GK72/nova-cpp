#!/usr/bin/env bash

set -euo pipefail
source "$(dirname "$(realpath "$0")")/common.sh"

echo "=== Stage: Unit Test ==="
ctest --test-dir "${BUILD_DIR}/libnova" \
    --build-config "${BUILD_TYPE}" \
    --output-on-failure \
    --label-exclude bench
