#!/usr/bin/env bash

# Version bumper tool
# Automatically bumps version based on git tag.
# Both in `nova.hh` and in `conanfile.py` files.
#
# Everything is patch bump unless "Feat".
# The library is at major version 0, meaning breaking changes are not followed
# in the versioning as the library is in experimental phase.

set -euo pipefail

version=$(sed --quiet 's/.*version = "\(.*\)"/\1/p' conanfile.py)
v_major=$(cut -d '.' -f 1 <<< "${version}")
v_minor=$(cut -d '.' -f 2 <<< "${version}")
v_patch=$(cut -d '.' -f 3 <<< "${version}")

echo "Previous version: ${version}"

commit_tag=$(git show --format="%s" --quiet |cut -d ':' -f 1)

if [[ "${commit_tag}" =~ (F|feat) ]]; then
    v_minor=$((v_minor+1))
    v_patch=0
else
    v_patch=$((v_patch+1))
fi

bumped_version="${v_major}.${v_minor}.${v_patch}"
echo "Bumped version: ${bumped_version}"

sed --in-place \
    -e "s/\(.*NovaVersionMajor = \).*;/\1${v_major};/" \
    -e "s/\(.*NovaVersionMinor = \).*;/\1${v_minor};/" \
    -e "s/\(.*NovaVersionPatch = \).*;/\1${v_patch};/" \
    include/nova/nova.hh

sed --in-place -e "s/\(.*version = \).*/\1\"${bumped_version}\"/" conanfile.py
