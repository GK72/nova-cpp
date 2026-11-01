#!/usr/bin/env bash
#
# Not part of `e2e.sh`: meaningful only on a Release build without sanitizers,
# e.g. `BUILD_TYPE=Release SANITIZERS= ci/build.sh && BUILD_TYPE=Release ci/bench.sh`.

set -euo pipefail
source "$(dirname "$(realpath "$0")")/common.sh"

echo "=== Stage: Benchmark ==="
ctest --test-dir "${BUILD_DIR}/libnova" \
    --build-config "${BUILD_TYPE}" \
    --output-on-failure \
    --label-regex bench \
    --verbose
