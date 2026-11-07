/**
 * Part of Nova C++ Library.
 *
 * @brief   Benchmarks of various CRC32 backends.
 *
 * Automatically benchmarks every implementation backend that is supported by
 * the CPU.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-07
 * @updated     2026-11-07
 * @reviewed    read
 * SPDX-License-Identifier: BSL-1.0
 */

#include <libnova/checksum.hpp>
#include <libnova/detail/crc32.hpp>

#include <benchmark/benchmark.h>
#include <fmt/format.h>

#include <cstddef>
#include <vector>

namespace {

    void crc32(benchmark::State& state, nova::detail::crc32_fn fn) {
        const auto size = static_cast<std::size_t>(state.range(0));
        std::vector<std::byte> data(size);

        for (std::size_t i = 0; i < size; ++i) {
            data[i] = static_cast<std::byte>(i * 151U);
        }

        for (auto _ : state) {
            auto crc = fn(data, 0xFFFFFFFFU);
            benchmark::DoNotOptimize(crc);
        }

        state.SetBytesProcessed(state.iterations() * state.range(0));
    }

} // namespace

// Every implementation the CPU supports
int main(int argc, char** argv) {
    for (const auto& backend : nova::detail::crc32_backends()) {
        benchmark::RegisterBenchmark(fmt::format("crc32/{}", backend.name), crc32, backend.fn)
            ->ArgName("bytes")
            ->RangeMultiplier(16)
            ->Range(16, 1 << 20);
    }

    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
