#!/usr/bin/env bash
#
# Enforces the NOLINT convention:
#
#   // NOLINT(check-a,check-b) | Reason the finding is waived.
#
# - The suppressed check(s) must be named: a bare `// NOLINT` silences every
#   clang-tidy check on the line, hiding unrelated findings.
# - A non-empty reason must follow ` | `, so a reviewer sees why the waiver is
#   acceptable.
#
# Applies to NOLINT, NOLINTNEXTLINE and NOLINTBEGIN. NOLINTEND only closes a
# BEGIN block (which carries the reason) and is exempt.

set -euo pipefail
source "$(dirname "$(realpath "$0")")/common.sh"

echo "=== Stage: NOLINT Check ==="

FILES=('*.cpp' '*.hpp' '*.cc' '*.hh')
# Any marker except NOLINTEND.
MARKER='NOLINT(NEXTLINE|BEGIN)?([^A-Z]|$)'
# Conforming marker: named checks, then ` | ` and a non-blank reason.
CONFORMING='NOLINT(NEXTLINE|BEGIN)?\([^)]+\) \| *[^ ]'

if violations=$(git -C "${WORKSPACE}" grep -nE "${MARKER}" -- "${FILES[@]}" | grep -vE "${CONFORMING}"); then
    echo "Non-conforming NOLINT found; use: // NOLINT(check-name) | Reason." >&2
    echo "${violations}" >&2
    exit 1
fi

echo "All NOLINT comments name their checks and give a reason."
