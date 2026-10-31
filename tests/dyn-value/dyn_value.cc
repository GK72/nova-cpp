/**
 * Function tests of reading `nova::dyn_value` documents from files.
 *
 * Run from this directory: the fixtures are next to this file. The case to
 * run is the first argument. The writing cases write into `$FT_OUT_DIR`.
 */

#include <libnova/dyn_value.hpp>
#include <libnova/error.hpp>
#include <libnova/main.hpp>
#include <libnova/utils.hpp>

#include <fmt/format.h>

#include <cstdlib>
#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <utility>

namespace {

    /**
     * @brief   Prints the failure if `condition` is false.
     */
    template <typename... Args>
    [[nodiscard]] auto check(bool condition, fmt::format_string<Args...> message, Args&&... args) -> bool {
        if (not condition) {
            fmt::println("Test failed: {}", fmt::format(message, std::forward<Args>(args)...));
        }
        return condition;
    }

    /**
     * @brief   `std::string_view::contains` is C++23.
     */
    [[nodiscard]] auto has(std::string_view text, std::string_view part) -> bool {
        return text.find(part) != std::string_view::npos;
    }

    [[nodiscard]] auto result(bool passed) -> int {
        return passed ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    [[nodiscard]] auto test_formats() -> int {
        const auto expected = nova::dyn_value::map({
            { "name", "x" },
            { "jobs", 8 },
            { "env", nova::dyn_value::map({ { "CC", "clang" } }) },
        });

        auto passed = true;
        for (const auto* file : { "doc.yaml", "doc.json", "doc.toml" }) {
            const auto doc = nova::dyn_value::read_from(file);
            if (not doc) {
                passed = check(false, "`{}`: {}", file, doc.error());
                continue;
            }
            passed = check(*doc == expected, "`{}` read into a different tree", file) and passed;
        }
        return result(passed);
    }

    [[nodiscard]] auto test_config_comments() -> int {
        const auto doc = nova::dyn_value::read_config("comments.yaml");
        if (not doc) {
            return result(check(false, "{}", doc.error()));
        }
        return result(check(*doc == nova::dyn_value::map({ }), "a config of only comments is not an empty map"));
    }

    /**
     * @brief   Whether reading failed with `code`, the detail starting with `file`.
     */
    [[nodiscard]] auto fails_with(
            const nova::expected<nova::dyn_value, nova::error>& doc,
            std::error_code code,
            std::string_view file
    ) -> bool {
        if (doc) {
            return check(false, "`{}` read, but it should have failed", file);
        }
        return check(doc.error().code == code, "unexpected error: {}", doc.error())
           and check(has(doc.error().detail, file), "the error does not name `{}`: {}", file, doc.error());
    }

    [[nodiscard]] auto test_config_list() -> int {
        const auto not_a_map = nova::make_error_code(nova::dyn_value::error_e::not_a_map);
        return result(fails_with(nova::dyn_value::read_config("list.yaml"), not_a_map, "list.yaml"));
    }

    [[nodiscard]] auto test_missing() -> int {
        const auto missing = std::make_error_code(std::errc::no_such_file_or_directory);
        return result(fails_with(nova::dyn_value::read_config("missing.yaml"), missing, "missing.yaml"));
    }

    [[nodiscard]] auto test_directory() -> int {
        const auto directory = std::make_error_code(std::errc::is_a_directory);
        return result(fails_with(nova::dyn_value::read_config("dir.yaml"), directory, "dir.yaml"));
    }

    [[nodiscard]] auto test_syntax_error() -> int {
        const auto syntax = nova::make_error_code(nova::dyn_value::error_e::syntax);
        return result(fails_with(nova::dyn_value::read_from("broken.json"), syntax, "broken.json"));
    }

    [[nodiscard]] auto test_value_message() -> int {
        try {
            std::ignore = nova::dyn_value::read_config("missing.yaml").value();
        } catch (const nova::exception& ex) {
            const auto message = std::string_view{ ex.what() };
            return result(
                check(has(message, "No such file or directory"), "unexpected message: {}", message)
                and check(has(message, "missing.yaml"), "the message does not name the file: {}", message)
            );
        }
        return result(check(false, "`value()` of a missing config did not throw"));
    }

    [[nodiscard]] auto out_dir() -> std::filesystem::path {
        return nova::getenv("FT_OUT_DIR").value();
    }

    [[nodiscard]] auto sample() -> nova::dyn_value {
        return nova::dyn_value::map({
            { "name", "x" },
            { "version", "1.10" },
            { "ratio", 0.5 },
            { "list", nova::dyn_value::seq({ 1, 2 }) },
            { "env", nova::dyn_value::map({ { "CC", "clang" } }) },
        });
    }

    /**
     * @brief   Whether `path` reads back as `expected`.
     */
    [[nodiscard]] auto reads_as(const std::filesystem::path& path, const nova::dyn_value& expected) -> bool {
        const auto doc = nova::dyn_value::read_from(path);
        if (not doc) {
            return check(false, "`{}`: {}", path.string(), doc.error());
        }
        return check(*doc == expected, "`{}` read back into a different tree", path.string());
    }

    [[nodiscard]] auto test_write_every_format() -> int {
        auto passed = true;
        for (const auto* extension : { "yaml", "yml", "json", "toml" }) {
            // A directory that does not exist yet
            const auto path = out_dir() / "every-format" / extension / fmt::format("doc.{}", extension);
            std::filesystem::remove_all(path.parent_path());

            const auto written = sample().write_to(path);
            if (not written) {
                passed = check(false, "`{}`: {}", path.string(), written.error());
                continue;
            }
            passed = reads_as(path, sample()) and passed;

            auto temp = path;
            temp += ".tmp";
            passed = check(not std::filesystem::exists(temp), "`{}` is left behind", temp.string()) and passed;
        }
        return result(passed);
    }

    [[nodiscard]] auto test_write_replace() -> int {
        const auto path = out_dir() / "replace.json";
        const auto update = nova::dyn_value::map({ { "name", "y" } });

        if (const auto written = sample().write_to(path); not written) {
            return result(check(false, "{}", written.error()));
        }
        if (const auto written = update.write_to(path); not written) {
            return result(check(false, "{}", written.error()));
        }
        return result(reads_as(path, update));
    }

    [[nodiscard]] auto test_write_unsupported() -> int {
        const auto path = out_dir() / "unsupported.toml";
        if (const auto written = sample().write_to(path); not written) {
            return result(check(false, "{}", written.error()));
        }

        const auto written = nova::dyn_value::map({ { "gone", nullptr } }).write_to(path);
        if (written) {
            return result(check(false, "a null was written to TOML"));
        }

        const auto unsupported = nova::make_error_code(nova::dyn_value::error_e::unsupported);
        return result(
            check(written.error().code == unsupported, "unexpected error: {}", written.error())
            and check(has(written.error().detail, path.string()), "the error does not name the file: {}", written.error())
            and reads_as(path, sample())
        );
    }

    [[nodiscard]] auto test_write_unknown_format() -> int {
        const auto path = out_dir() / "doc.ini";
        const auto written = sample().write_to(path);
        if (written) {
            return result(check(false, "`{}` was written", path.string()));
        }

        const auto unknown = nova::make_error_code(nova::dyn_value::error_e::unknown_format);
        return result(
            check(written.error().code == unknown, "unexpected error: {}", written.error())
            and check(not std::filesystem::exists(path), "`{}` was created", path.string())
        );
    }

} // namespace

auto entrypoint(auto args) -> int {
    if (args.size() != 2) {
        fmt::println("Usage: ft-dyn-value <case>");
        return EXIT_FAILURE;
    }

    static const auto cases = std::map<std::string_view, std::function<int()>>{
        { "formats",         test_formats },
        { "config-comments", test_config_comments },
        { "config-list",     test_config_list },
        { "missing",         test_missing },
        { "directory",       test_directory },
        { "syntax-error",    test_syntax_error },
        { "value-message",   test_value_message },
        { "write-every-format",  test_write_every_format },
        { "write-replace",       test_write_replace },
        { "write-unsupported",   test_write_unsupported },
        { "write-unknown-format", test_write_unknown_format },
    };

    const auto found = cases.find(std::string_view{ args[1] });
    if (found == cases.end()) {
        fmt::println("Unknown case `{}`", args[1]);
        return EXIT_FAILURE;
    }
    return found->second();
}

NOVA_MAIN(entrypoint);
