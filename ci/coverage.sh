#!/usr/bin/env bash
#
# Coverage reports of the unit and integration tests; needs a `COVERAGE=ON`
# build (default for Debug), the test stages to have run, and `gcovr`.

set -euo pipefail
source "$(dirname "$(realpath "$0")")/common.sh"

COVERAGE_DIR="${WORKSPACE}/build/coverage"

echo "=== Stage: Coverage ==="
mkdir -p "${COVERAGE_DIR}"

# Coverage of what runs: the unit tests and the integration tests, whose
# installed headers are mapped back to the sources (`function-test.sh`). Only
# the library is reported, not the tests or code that is built but never run
# (benchmarks, in-tree examples).
workspace_regex=$(sed 's/[][\.*^$+?(){}|]/\\&/g' <<< "${WORKSPACE}")
gcovr_args=(
    "${BUILD_DIR}/libnova"
    "${BUILD_DIR}/tests"
    --root "${WORKSPACE}"
    --filter "${workspace_regex}/libnova/libnova/"
    --exclude '.*\.(test|bench|ex)\.cpp$'
    --exclude '.*/test_utils\.hpp$'
)
gcovr "${gcovr_args[@]}" --cobertura --output "${COVERAGE_DIR}/cobertura.xml"
gcovr "${gcovr_args[@]}" --html-details --output "${COVERAGE_DIR}/nova.html"
