#include <libnova/log.hpp>

#include <benchmark/benchmark.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>

static void init_log() {
    auto null_sink = std::make_shared<spdlog::sinks::null_sink_mt>();
    null_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%f %z] [%n @%t] %^[%l]%$ %v");

    nova::topic_log::create_multi(
        {
            "bench",
        },
        { null_sink }
    );

    nova::log::init();
    spdlog::set_default_logger(spdlog::get("bench"));
}

static void log(benchmark::State& state) {
    for (auto _ : state) {
        nova::log::info("bench", "Benching...");
    }
}

static void log_filtered(benchmark::State& state) {
    for (auto _ : state) {
        nova::log::debug("bench", "Benching...");
    }
}

static void topic_log(benchmark::State& state) {
    for (auto _ : state) {
        nova::topic_log::info("bench", "Benching...");
    }
}

static void topic_log_filtered(benchmark::State& state) {
    for (auto _ : state) {
        nova::topic_log::debug("bench", "Benching...");
    }
}

BENCHMARK(log);
BENCHMARK(log_filtered);
BENCHMARK(log)->Threads(2);
BENCHMARK(log_filtered)->Threads(2);
BENCHMARK(topic_log);
BENCHMARK(topic_log_filtered);
BENCHMARK(topic_log)->Threads(2);
BENCHMARK(topic_log_filtered)->Threads(2);
BENCHMARK(topic_log)->Threads(4);
BENCHMARK(topic_log_filtered)->Threads(4);

int main(int argc, char** argv) {
   init_log();
   ::benchmark::Initialize(&argc, argv);
   ::benchmark::RunSpecifiedBenchmarks();
}
