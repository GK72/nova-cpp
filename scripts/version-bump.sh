#!/usr/bin/env bash

# Version bumper tool
# Automatically bumps version based on git tag.
# The version lives in `libnova/VERSION` only, CMake and Conan read it.
#
# Everything is patch bump unless "Feat".
# The library is at major version 0, meaning breaking changes are not followed
# in the versioning as the library is in experimental phase.

set -euo pipefail

version=$(< libnova/VERSION)
v_major=$(cut -d '.' -f 1 <<< "${version}")
v_minor=$(cut -d '.' -f 2 <<< "${version}")
v_patch=$(cut -d '.' -f 3 <<< "${version}")

echo "Previous version: ${version}"

commit_tag=$(git show --format="%s" --quiet |cut -d ':' -f 1)

if [[ "${commit_tag}" =~ (F|f)eat ]]; then
    v_minor=$((v_minor+1))
    v_patch=0
else
    v_patch=$((v_patch+1))
fi

bumped_version="${v_major}.${v_minor}.${v_patch}"
echo "Bumped version: ${bumped_version}"

echo "${bumped_version}" > libnova/VERSION
