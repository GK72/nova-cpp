#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(realpath "${0%%/*}")"
PROJECT_DIR=$(git -C "${SCRIPT_DIR}" rev-parse --show-toplevel)
BUILD_DIR="${PROJECT_DIR}/build"
JOBS=$(($(nproc) / 2))

function usage() {
    >&2 cat << EOF
Usage: $0 [-c|--clean]

Compiles all targets with sanitizers and runs unit and function tests.

Dependencies:
- cmake
- baldr (cargo install baldr)
EOF
}

f_clean=false

function parse_args() {
    while [[ $# -gt 0 ]]; do
      case "$1" in
        -c|--clean)     f_clean=true;                       shift;;
        -h|--help)      usage;                              shift;      exit 0 ;;
        *)              msg "Invalid option: $1"; usage;    shift;      exit 1 ;;
      esac
    done
}

function msg() {
    >&2 echo "$*"
}

function build-and-test() {
    baldr --project "${PROJECT_DIR}" -b Debug -t all -j "${JOBS}" --config "${PROJECT_DIR}/.baldr-clang.yaml"
    ctest --output-on-failure --test-dir "${BUILD_DIR}/debug-clang++"

    baldr --project "${PROJECT_DIR}" -b Debug -t all -j "${JOBS}" -DNOVA_EXPERIMENTAL_FEATURE_SET=ON
    ctest --output-on-failure --test-dir "${BUILD_DIR}/debug"
}

function main() {
    if [[ $f_clean == true ]]; then
        msg "Clean build"
        rm -rf "${PROJECT_DIR}/build"
    fi

    build-and-test
}

parse_args "$@"
main
