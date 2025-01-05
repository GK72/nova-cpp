#include <benchmark/benchmark.h>

#include <nova/data.hh>
#include <nova/random.hh>
#include <nova/types.hh>

#include <cstdint>
#include <limits>

constexpr auto ByteArrayLength = 100'000'000;

template <typename T>
static void integer(benchmark::State& state) {
    const auto length = static_cast<std::size_t>(state.range(0));
    auto value = nova::random().number<T>(nova::range<T>{ 1, std::numeric_limits<T>::max() });

    for (auto _ : state) {
        auto ser = nova::serializer_context{ ByteArrayLength };
        for (std::size_t i = 0; i < length; ++i) {
            ser(value);
        }
        auto data = ser.data();
        benchmark::DoNotOptimize(data);
    }
}

static void string(benchmark::State& state) {
    const auto length = static_cast<std::size_t>(state.range(0));
    auto value = nova::random().string<nova::ascii_distribution>(1);

    for (auto _ : state) {
        auto ser = nova::serializer_context{ ByteArrayLength };
        for (std::size_t i = 0; i < length; ++i) {
            ser(value);
        }
        auto data = ser.data();
        benchmark::DoNotOptimize(data);
    }
}

BENCHMARK(string)->RangeMultiplier(4)->Range(16, 2 << 14);
BENCHMARK(integer<std::uint8_t>)->RangeMultiplier(4)->Range(16, 2 << 14);
BENCHMARK(integer<std::uint16_t>)->RangeMultiplier(4)->Range(16, 2 << 14);
BENCHMARK(integer<std::uint32_t>)->RangeMultiplier(4)->Range(16, 2 << 14);
BENCHMARK(integer<std::uint64_t>)->RangeMultiplier(4)->Range(16, 2 << 14);

BENCHMARK_MAIN();
