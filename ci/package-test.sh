#!/usr/bin/env bash
#
# Consumes the Conan package (`package.sh`) with `tests/package`: CMake
# modules, linking and dependencies as a downstream project sees them.

set -euo pipefail
source "$(dirname "$(realpath "$0")")/common.sh"
source "${WORKSPACE}/scripts/release.sh"

REFERENCE="nova/$(version_at HEAD)"

echo "=== Stage: Package Test (${REFERENCE}) ==="
conan test "${WORKSPACE}/tests/package" "${REFERENCE}" \
    --build=missing \
    --settings compiler.cppstd=20
