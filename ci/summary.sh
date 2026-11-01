#!/usr/bin/env bash

set -euo pipefail
source "$(dirname "$(realpath "$0")")/common.sh"
source "${WORKSPACE}/scripts/release.sh"

echo "=== Stage: Summary ==="
ls -l "${PACKAGE_DIR}"/*.rpm
conan list "nova/$(version_at HEAD):*"
