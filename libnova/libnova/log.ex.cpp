#include <libnova/log.hpp>

#include <spdlog/sinks/ansicolor_sink.h>

#include <memory>

namespace {

void log_init() {
    auto stderr_sink = std::make_shared<spdlog::sinks::ansicolor_stderr_sink_mt>();

    stderr_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%f %z] [%n @%t] %^[%l]%$ %v");

    nova::topic_log::create_multi(
        {
            "ex",
            "ex2",
        },
        { stderr_sink }
    );

    nova::log::init();
}

} // namespace

int main() {
    log_init();

    nova::topic_log::info("ex", "This is an example application to test logging");
    nova::topic_log::info("ex", "Use `SPDLOG_LEVEL=trace` environment variable to see every logs");

    nova::topic_log::critical("ex", "Critical log");
    nova::topic_log::error("ex", "Error log");
    nova::topic_log::warn("ex", "Warning log");
    nova::topic_log::debug("ex", "Debug log");
    nova::topic_log::trace("ex", "Trace log");
    nova::topic_log::devel("ex", "Devel log visible only in DEBUG build");
}
