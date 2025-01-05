#include <nova/io.hh>
#include <nova/log.hh>
#include <nova/main.hh>

#include <spdlog/sinks/basic_file_sink.h>

#include <cstdlib>

constexpr auto logfile = "nova-test.log";

namespace {

[[nodiscard]] auto check() -> bool {
    const auto content = nova::read_file(logfile);
    if (not content.has_value()) {
        fmt::println("Error: {}", content.error().message);
        return false;
    }

    auto pos = content->find("Hello Nova 1");
    if (pos == std::string::npos) {
        fmt::println("Test failed!\nInfo level log does not exist for `nova`");
        return false;
    }

    pos = content->find("Hello Nova 2");
    if (pos != std::string::npos) {
        fmt::println("Test failed!\nDebug level log does exist for `nova`, but it should not.");
        return false;
    }

    pos = content->find("Hello Nova 3");
    if (pos == std::string::npos) {
        fmt::println("Test failed!\nDebug level log does not exist for `nova-2`");
        return false;
    }

    return true;
}

} // namespace

auto entrypoint([[maybe_unused]] auto args) -> int {
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logfile);

    nova::topic_log::create_multi(
        { "nova", "nova2" },
        { file_sink }
    );

    nova::log::init();

    nova::topic_log::info("nova", "Hello Nova 1");
    nova::topic_log::debug("nova", "Hello Nova 2");
    nova::topic_log::debug("nova2", "Hello Nova 3");

    file_sink->flush();

    if (check()) {
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

MAIN(entrypoint);
