# Sourced by every stage. Defaults can be overridden from the environment.
#
#   WORKSPACE   Project checkout the stages work on (default: this repository).
#   BUILD_TYPE  Debug or Release (default: Debug).
#   SANITIZERS  asan, tsan or empty to disable (default: asan).
#   COVERAGE    ON to instrument for coverage (default: ON for Debug).

CI_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
PROJECT_DIR=$(git -C "${CI_DIR}" rev-parse --show-toplevel)

: "${WORKSPACE:=${PROJECT_DIR}}"
: "${BUILD_TYPE:=Debug}"
: "${SANITIZERS=asan}"
if [[ "${BUILD_TYPE}" == Debug ]]; then
    : "${COVERAGE:=ON}"
else
    : "${COVERAGE:=OFF}"
fi
export WORKSPACE BUILD_TYPE SANITIZERS COVERAGE

BUILD_DIR="${WORKSPACE}/build/${BUILD_TYPE}"
INSTALL_DIR="${WORKSPACE}/install/${BUILD_TYPE}"
PACKAGE_DIR="${WORKSPACE}/package"
CONAN_PROVIDER="${WORKSPACE}/conan_provider.cmake"
