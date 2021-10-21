#!/usr/bin/env bash

set -e

PATH_REPO_ROOT=$(git rev-parse --show-toplevel)
PATH_BUILD_TESTS="${PATH_REPO_ROOT}/build/tests"
PATH_TEST_RESULTS="${PATH_REPO_ROOT}/build/artifacts/test_results"
PATH_REFERENCE_PATTERNS="${PATH_REPO_ROOT}/tests/expected_output"
TEST_TARGETS=$(sed -n 's/add_executable(\(.* \).*/\1/p' "${PATH_REPO_ROOT}/tests/CMakeLists.txt")

mkdir -p "${PATH_BUILD_TESTS}" && cd "${PATH_BUILD_TESTS}"
cmake -DCMAKE_BUILD_TYPE=release "${PATH_REPO_ROOT}"

for TARGET in ${TEST_TARGETS}; do
    make -j ${TARGET}
done

mkdir -p "${PATH_TEST_RESULTS}"

for TARGET in ${TEST_TARGETS}; do
    echo "Running executable ${TARGET}"
    tests/${TARGET} > "${PATH_TEST_RESULTS}/${TARGET}"
done

echo "Test results generated at ${PATH_TEST_RESULTS}"

for TARGET in ${TEST_TARGETS}; do
    echo "Comparing results ${TARGET}"
    while read -r LINE; do
        PATTERN=$(cut -d'	' -f1 <<< "${LINE}")
        TEXT=$(cut -d'	' -f2 <<< "${LINE}")

        set +e
        sed -n "/^${PATTERN}\$/!{q99}" <<< "${TEXT}"
        if [[ $? -eq 99 ]]; then
            echo "Pattern failed: '${PATTERN}' in '${TEXT}'"
        fi
        set -e
    done <<< $(paste "${PATH_REFERENCE_PATTERNS}/${TARGET}.txt" "${PATH_TEST_RESULTS}/${TARGET}")
done

echo "Test outputs successfully matched the expected patterns."
