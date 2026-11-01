#!/usr/bin/env bash
#
# All packages of the Release build: RPM (Linux only) and Conan. Tested by
# `rpm-test.sh` and `package-test.sh`.

set -euo pipefail
export BUILD_TYPE=Release
source "$(dirname "$(realpath "$0")")/common.sh"

echo "=== Stage: Package ==="
if [[ "$(uname -s)" == Linux ]]; then
    cpack --config "${BUILD_DIR}/libnova/CPackConfig.cmake" -B "${PACKAGE_DIR}"
else
    echo "RPM: skipped, Linux only."
fi

conan create "${WORKSPACE}/libnova" \
    --test-folder="" \
    --build=missing \
    --settings compiler.cppstd=20
