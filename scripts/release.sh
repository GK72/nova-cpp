# Shared by `ci/check-version-bump.sh` and `ci/tag-release.sh`.

# Paths whose changes are release-relevant: they can change the library or how
# it is built and packaged, so they warrant a version bump and count towards
# "is there something new to release?".
RELEASE_RELEVANT_PATHS=(
    'libnova'
)

# Version of the library at the given ref; the only place that knows where the
# version lives.
version_at() {
    git -C "${WORKSPACE}" show "$1:libnova/conanfile.py" \
        | sed -n 's/^ *version = "\([0-9.]*\)".*/\1/p'
}
