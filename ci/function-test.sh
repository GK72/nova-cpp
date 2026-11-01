#!/usr/bin/env bash
#
# Integration tests against the installed package (`install.sh`). Built with
# the same coverage setting: an instrumented library needs `--coverage` when
# linking (`__gcov_*`), and the integration tests count towards coverage.

set -euo pipefail
source "$(dirname "$(realpath "$0")")/common.sh"

echo "=== Stage: Function Test ==="
cmake -S "${WORKSPACE}/tests" -B "${BUILD_DIR}/tests" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DSANITIZERS="${SANITIZERS}" \
    -DCOVERAGE="${COVERAGE}" \
    -DCMAKE_PREFIX_PATH="${INSTALL_DIR}" \
    -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES="${CONAN_PROVIDER}"

cmake --build "${BUILD_DIR}/tests" --config "${BUILD_TYPE}"
ctest --test-dir "${BUILD_DIR}/tests" --build-config "${BUILD_TYPE}" --output-on-failure
