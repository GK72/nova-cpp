#!/usr/bin/env bash

function msg() {
    >&2 echo "$*"
}

set -euo pipefail

SCRIPT_DIR="$(realpath "${0%%/*}")"
PROJECT_DIR=$(git rev-parse --show-toplevel -C "${SCRIPT_DIR}")
JOBS=$(($(nproc) / 2))

function usage() {
    >&2 cat << EOF
Usage: $0 [-d|--diff]

Run clang-tidy for all or changed source files in parallel.

Dependencies:
- clang-tidy
- git
- parallel
EOF
}

f_diff=false

function parse_args() {
    while [[ $# -gt 0 ]]; do
      case "$1" in
        -d|--diff)      f_diff=true;                        shift;;
        -h|--help)      usage;                              shift;      exit 0 ;;
        *)              msg "Invalid option: $1"; usage;    shift;      exit 1 ;;
      esac
    done
}

function run() {
    local file="$1"
    clang-tidy "${file}"
}

function source-files() {
    if [[ ${f_diff} == true ]]; then
        git diff --name-only --diff-filter=ACM HEAD~1 -- **/*.{cc,hh}
    else
        find "${PROJECT_DIR}" -type f -name '*.cc' -or -name '*.hh'
    fi | grep -vE 'nova.hh'
}

export -f run

function main() {
    local files
    mapfile -t files < <(source-files)

    parallel -j "${JOBS}" run ::: "${files[@]}"
}

parse_args "$@"
main
