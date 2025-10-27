#include <libnova/data.hpp>
#include <libnova/random.hpp>
#include <libnova/types.hpp>

#include <benchmark/benchmark.h>

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

static void string_consecutive_writes(benchmark::State& state) {
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

static void vector_consecutive_writes(benchmark::State& state) {
    const auto length = static_cast<std::size_t>(state.range(0));
    auto x = nova::random().string<nova::ascii_distribution>(1);
    auto value = nova::data_view(x).to_vec();

    for (auto _ : state) {
        auto ser = nova::serializer_context{ ByteArrayLength };
        for (std::size_t i = 0; i < length; ++i) {
            ser(value);
        }
        auto data = ser.data();
        benchmark::DoNotOptimize(data);
    }
}

static void string_size(benchmark::State& state) {
    const auto length = static_cast<std::size_t>(state.range(0));
    auto value = nova::random().string<nova::ascii_distribution>(length);

    for (auto _ : state) {
        auto ser = nova::serializer_context{ ByteArrayLength };
        ser(value);
        auto data = ser.data();
        benchmark::DoNotOptimize(data);
    }
}

static void vector_size(benchmark::State& state) {
    const auto length = static_cast<std::size_t>(state.range(0));
    auto x = nova::random().string<nova::ascii_distribution>(length);
    auto value = nova::data_view(x).to_vec();

    for (auto _ : state) {
        auto ser = nova::serializer_context{ ByteArrayLength };
        ser(value);
        auto data = ser.data();
        benchmark::DoNotOptimize(data);
    }
}

struct custom_t {
    std::uint8_t value;
};

namespace nova {

template <>
struct serializer<custom_t> {
    void operator()(serializer_context& ser, const custom_t& x) {
        ser(x.value);
    }
};

} // namespace nova

static void vector_size__custom_type(benchmark::State& state) {
    const auto length = static_cast<std::size_t>(state.range(0));
    auto xs = std::vector<custom_t>{ };

    for (std::size_t i = 0; i < length; ++i) {
        const auto range = nova::range<std::uint8_t>{ 0, 255 };
        xs.push_back(custom_t{ nova::random().number(range) });
    }

    for (auto _ : state) {
        auto ser = nova::serializer_context{ ByteArrayLength };
        ser(xs);
        auto data = ser.data();
        benchmark::DoNotOptimize(data);
    }
}

BENCHMARK(string_consecutive_writes)->RangeMultiplier(4)->Range(16, 2 << 14);
BENCHMARK(vector_consecutive_writes)->RangeMultiplier(4)->Range(16, 2 << 14);
BENCHMARK(string_size)->RangeMultiplier(4)->Range(16, 2 << 14);
BENCHMARK(vector_size)->RangeMultiplier(4)->Range(16, 2 << 14);
BENCHMARK(vector_size__custom_type)->RangeMultiplier(4)->Range(16, 2 << 14);
BENCHMARK(integer<std::uint8_t>)->RangeMultiplier(4)->Range(16, 2 << 14);
BENCHMARK(integer<std::uint16_t>)->RangeMultiplier(4)->Range(16, 2 << 14);
BENCHMARK(integer<std::uint32_t>)->RangeMultiplier(4)->Range(16, 2 << 14);
BENCHMARK(integer<std::uint64_t>)->RangeMultiplier(4)->Range(16, 2 << 14);

BENCHMARK_MAIN();
