#!/usr/bin/env bash

set -euo pipefail
source "$(dirname "$(realpath "$0")")/common.sh"

echo "=== Stage: Build (${BUILD_TYPE}, sanitizers: ${SANITIZERS:-none}, coverage: ${COVERAGE}) ==="
# The CPack configuration only matters for `package.sh` (Release).
cmake -S "${WORKSPACE}/libnova" -B "${BUILD_DIR}/libnova" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DBUILD_TESTING=ON \
    -DSANITIZERS="${SANITIZERS}" \
    -DCOVERAGE="${COVERAGE}" \
    -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES="${CONAN_PROVIDER}" \
    -DCMAKE_PROJECT_INCLUDE="${WORKSPACE}/config-package-suse.cmake"

cmake --build "${BUILD_DIR}/libnova" --config "${BUILD_TYPE}"
