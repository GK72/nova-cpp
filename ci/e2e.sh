#!/usr/bin/env bash
#
# Runs the whole pipeline locally on a fresh clone of the committed HEAD.

set -euo pipefail

SCRIPT_DIR="$(dirname "$(realpath "$0")")"
PROJECT_DIR=$(git -C "${SCRIPT_DIR}" rev-parse --show-toplevel)

export WORKSPACE=/tmp/workspace/nova

"${PROJECT_DIR}/ci/prepare.sh"
cd "${WORKSPACE}"

# Stages run from the clone, i.e. the committed versions of the scripts.
"${WORKSPACE}/ci/pre-checks.sh"

test_configuration() {
    "${WORKSPACE}/ci/build.sh"
    "${WORKSPACE}/ci/unit-test.sh"
    "${WORKSPACE}/ci/install.sh"
    "${WORKSPACE}/ci/function-test.sh"
    "${WORKSPACE}/ci/examples.sh"
}

# Debug with sanitizers and coverage, then Release: what is shipped.
test_configuration
(
    export BUILD_TYPE=Release SANITIZERS= COVERAGE=OFF
    test_configuration
)

"${WORKSPACE}/ci/package.sh"
"${WORKSPACE}/ci/rpm-test.sh"
"${WORKSPACE}/ci/package-test.sh"
"${WORKSPACE}/ci/tag-release.sh"
"${WORKSPACE}/ci/summary.sh"
