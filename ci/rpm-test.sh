#!/usr/bin/env bash
#
# Whether the RPM package (`package.sh`) installs in an openSUSE container.

set -euo pipefail
source "$(dirname "$(realpath "$0")")/common.sh"

echo "=== Stage: RPM Test ==="
docker build \
    --tag local/nova:suse \
    --build-context package="${PACKAGE_DIR}" \
    --file "${WORKSPACE}/env/suse.Dockerfile" \
    "${WORKSPACE}/env"
