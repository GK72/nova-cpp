#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(dirname "$(realpath "$0")")"

"${SCRIPT_DIR}/check-version-bump.sh"
"${SCRIPT_DIR}/check-nolint.sh"
