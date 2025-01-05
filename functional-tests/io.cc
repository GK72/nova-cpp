#include <nova/data.hh>
#include <nova/io.hh>
#include <nova/main.hh>

#include <fmt/core.h>

#include <cstddef>
#include <cstdlib>
#include <span>
#include <string>
#include <vector>

using namespace std::string_literals;

namespace {

    [[nodiscard]] int test_invalid_file(const std::string& name) {
        const auto file = nova::read_file(name);
        if (file.has_value()) {
            fmt::println("Test failed!\nFile exists, but it should not!");
            return EXIT_FAILURE;
        }

        if (file.error().message.ends_with("nothing.txt is not a regular file!")) {
            return EXIT_SUCCESS;
        }

        fmt::println(
            "Test failed!\n"
            "Unexpected error message:\n{}",
            file.error().message
        );

        return EXIT_FAILURE;

    }

    [[nodiscard]] int test_read_text(const std::string& name) {
        const auto file = nova::read_file(name);
        if (not file.has_value()) {
            fmt::println("Error: {}", file.error().message);
            return EXIT_FAILURE;
        }

        if (const auto expected = "Hello IO\n"s; *file != expected) {                               // NOLINT(misc-include-cleaner) | Clang why are you like this? `#include <string>`
            fmt::println(
                "Test failed!\n"
                "Expected:\n`{}`\n"
                "Actual:\n`{}`",
                expected,
                *file
            );

            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    [[nodiscard]] int test_read_bin(const std::string& name) {
        const auto file = nova::read_bin(name);
        if (not file.has_value()) {
            fmt::println("Error: {}", file.error().message);
            return EXIT_FAILURE;
        }

        const auto expected = std::vector{ {
            std::byte{ 0x00 },
            std::byte{ 0x01 },
            std::byte{ 0x10 },
            std::byte{ 0xff }
        } };

        if (*file != expected) {
            fmt::println(
                "Test failed!\n"
                "Expected:{}\n"
                "Actual: {}`",
                nova::data_view(std::span(expected)).as_hex_string(),
                nova::data_view(std::span(*file)).as_hex_string()
            );

            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

} // namespace

auto entrypoint([[maybe_unused]] auto args) -> int {
    if (args.size() == 1) {
        fmt::println("Error: no file were given!");
        return EXIT_FAILURE;
    }

    const auto filename = std::string{ args[1] };
    if (filename == "nothing.txt") {
        return test_invalid_file(filename);
    }

    if (filename.ends_with("txt")) {
        return test_read_text(filename);
    }

    if (filename.ends_with("bin")) {
        return test_read_bin(filename);
    }

    fmt::println("File extension did not match with `txt` or `bin`!");
    return EXIT_FAILURE;
}

MAIN(entrypoint);
