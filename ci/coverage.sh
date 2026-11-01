#!/usr/bin/env bash
#
# Coverage reports of the unit and integration tests; needs a `COVERAGE=ON`
# build (default for Debug), the test stages to have run, and `gcovr`.

set -euo pipefail
source "$(dirname "$(realpath "$0")")/common.sh"

COVERAGE_DIR="${WORKSPACE}/build/coverage"

echo "=== Stage: Coverage ==="
mkdir -p "${COVERAGE_DIR}"
gcovr_args=(
    "${BUILD_DIR}"
    --root "${WORKSPACE}"
    --exclude '.*test.*'
    --exclude '.*_deps.*'
)
gcovr "${gcovr_args[@]}" --cobertura --output "${COVERAGE_DIR}/cobertura.xml"
gcovr "${gcovr_args[@]}" --html-details --output "${COVERAGE_DIR}/nova.html"
