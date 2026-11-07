#!/usr/bin/env bash
#
# Not part of `e2e.sh`: meaningful only on a Release build without sanitizers,
# e.g. `BUILD_TYPE=Release SANITIZERS= ci/build.sh && BUILD_TYPE=Release ci/bench.sh`.
#
# Runs every benchmark (or the modules given as arguments) and writes the
# results to `doc/bench/bench-<module>.json`. `tools/bench-report.py` turns
# them into the tables of the performance report.
#
#   BENCHMARK_MIN_TIME      Minimum time of one repetition (default: 1s).
#   BENCHMARK_REPETITIONS   Repetitions, the base of the variance (default: 10).

set -euo pipefail
source "$(dirname "$(realpath "$0")")/common.sh"

: "${BENCHMARK_MIN_TIME:=1s}"
: "${BENCHMARK_REPETITIONS:=10}"
export BENCHMARK_MIN_TIME BENCHMARK_REPETITIONS
export BENCHMARK_OUT_FORMAT=json

# The log benchmarks measure filtering `debug` out
export SPDLOG_LEVEL=info

BENCH_DIR="${BUILD_DIR}/libnova/libnova"
OUT_DIR="${PROJECT_DIR}/doc/bench"

modules=("$@")
if [[ ${#modules[@]} -eq 0 ]]; then
    for bench in "${BENCH_DIR}"/bench-*; do
        modules+=("${bench##*/bench-}")
    done
fi

echo "=== Stage: Benchmark ==="
for module in "${modules[@]}"; do
    echo "--- ${module} ---"
    BENCHMARK_OUT="${OUT_DIR}/bench-${module}.json" \
        "${BENCH_DIR}/bench-${module}" --benchmark_display_aggregates_only=true
done
