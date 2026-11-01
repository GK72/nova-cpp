#!/usr/bin/env bash
#
# Checks that the version was bumped if a release-relevant path changed since
# the base branch.
#
# The base to compare against is, in order: $CI_BASE_REF (manual override),
# 'origin/<GITHUB_BASE_REF>' (set by GitHub Actions for pull requests),
# 'origin/main', then 'main'. If none resolves, the check is skipped, not
# failed - there is nothing meaningful to compare against.

set -euo pipefail
source "$(dirname "$(realpath "$0")")/common.sh"
source "${WORKSPACE}/scripts/release.sh"

echo "=== Stage: Version Bump Check ==="

resolve_base() {
    local candidates=(
        "${CI_BASE_REF:-}"
        "${GITHUB_BASE_REF:+origin/${GITHUB_BASE_REF}}"
        "origin/main"
        "main"
    )
    for ref in "${candidates[@]}"; do
        [[ -z "${ref}" ]] && continue
        if git -C "${WORKSPACE}" rev-parse --verify --quiet "${ref}" >/dev/null; then
            echo "${ref}"
            return 0
        fi
    done
    return 1
}

if ! BASE="$(resolve_base)"; then
    echo "No base ref found (checked \$CI_BASE_REF, origin/\$GITHUB_BASE_REF, origin/main, main); skipping."
    exit 0
fi

MERGE_BASE=$(git -C "${WORKSPACE}" merge-base "${BASE}" HEAD)

if git -C "${WORKSPACE}" diff --quiet "${MERGE_BASE}" HEAD -- "${RELEASE_RELEVANT_PATHS[@]}"; then
    echo "No release-relevant changes since ${BASE} (${MERGE_BASE:0:12}); no version bump required."
    exit 0
fi

BASE_VERSION=$(version_at "${MERGE_BASE}")
HEAD_VERSION=$(version_at HEAD)

if [[ "${BASE_VERSION}" == "${HEAD_VERSION}" ]]; then
    echo "Release-relevant changes found since ${BASE} (${MERGE_BASE:0:12}), but the version was not bumped (${HEAD_VERSION})." >&2
    echo "Run scripts/version-bump.sh and commit the result." >&2
    exit 1
fi

echo "Version bumped since ${BASE} (${MERGE_BASE:0:12}): ${BASE_VERSION} -> ${HEAD_VERSION}."
