#include BUILD_INFO_HEADER
#include <libnova/version.hpp>

#include <fmt/format.h>

#include <cstdlib>
#include <span>
#include <string>
#include <string_view>

namespace {

auto failures = 0;

void check(bool condition, std::string_view what) {
    if (not condition) {
        fmt::println(stderr, "FAILED: {}", what);
        ++failures;
    }
}

} // namespace

auto main(int argc, char* argv[]) -> int {
    const auto args = std::span(argv, static_cast<std::size_t>(argc));
    if (args.size() != 3) {
        fmt::println(stderr, "Usage: {} <expected hash> <expected branch>", args[0]);
        return EXIT_FAILURE;
    }

    const auto expected_hash = std::string_view{ args[1] };
    const auto expected_branch = std::string_view{ args[2] };

    fmt::println("consumer: {}", ft::full_version());
    fmt::println("nova:     {}", nova::library_full_version());

    check(ft::version() == "1.2.3", "version");
    check(ft::version_major() == 1, "major");
    check(ft::version_minor() == 2, "minor");
    check(ft::version_patch() == 3, "patch");
    check(ft::git_hash() == expected_hash, "hash");
    check(ft::git_branch() == expected_branch, "branch");

    const auto expected_full = fmt::format(
        "1.2.3+{}.{}{}",
        expected_hash,
        expected_branch,
        ft::git_dirty() ? ".dirty" : ""
    );
    check(ft::full_version() == expected_full, "full version");

    check(nova::library_full_version().starts_with(std::string{ nova::library_version() } + "+"), "nova full version");

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
