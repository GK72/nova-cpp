#!/usr/bin/env bash
#
# Examples against the installed package (`install.sh`), with the same
# coverage setting (see `function-test.sh`).

set -euo pipefail
source "$(dirname "$(realpath "$0")")/common.sh"

echo "=== Stage: Examples ==="
cmake -S "${WORKSPACE}/examples" -B "${BUILD_DIR}/examples" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DSANITIZERS="${SANITIZERS}" \
    -DCOVERAGE="${COVERAGE}" \
    -DCMAKE_PREFIX_PATH="${INSTALL_DIR}" \
    -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES="${CONAN_PROVIDER}"

cmake --build "${BUILD_DIR}/examples" --config "${BUILD_TYPE}"
