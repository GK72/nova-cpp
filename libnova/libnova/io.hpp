/**
 * Part of Nova C++ Library.
 *
 * Input/Output related functionalities.
 */

#pragma once

#include <libnova/error.hpp>
#include <libnova/expected.hpp>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <ios>
#include <istream>
#include <sstream>
#include <string>
#include <system_error>
#include <type_traits>
#include <vector>

namespace nova {

namespace detail {

    /**
     * @brief   Default parser which returns the whole file as one string.
     */
    struct def_parser {
        [[nodiscard]] auto operator()(std::istream& inf) -> std::string {
            std::stringstream ss;
            for (std::string line; std::getline(inf, line); ) {
                ss << line << '\n';
            }
            return ss.str();
        }
    };

    /**
     * @brief   Default parser which returns the whole file as one vector.
     */
    struct def_bin_parser {
        [[nodiscard]] auto operator()(std::istream& inf) -> std::vector<std::byte> {
            inf.seekg(0, std::ios::end);
            const std::streamsize size = inf.tellg();
            inf.seekg(0);

            auto ret = std::vector<std::byte>(static_cast<std::size_t>(size));
            inf.read(reinterpret_cast<char*>(ret.data()), size);                                    // NOLINT(*reinterpret-cast) | Conversion between `std::byte` and `char` is allowed

            return ret;
        }
    };

    /**
     * @brief   Validate path and return with it if it points to a file.
     */
    [[nodiscard]] inline
    auto fs_path(const std::string& path) -> expected<std::filesystem::path, error> {
        auto fs = std::filesystem::path(path);
        const auto status = std::filesystem::status(fs);
        if (std::filesystem::is_regular_file(status)) {
            return fs;
        }

        const auto reason =
            not std::filesystem::exists(status) ? std::errc::no_such_file_or_directory
            : std::filesystem::is_directory(status) ? std::errc::is_a_directory
            : std::errc::invalid_argument;

        return wrap_error(std::make_error_code(reason), std::filesystem::absolute(fs).string());
    }

} // namespace detail

/**
 * @brief   A parser that puts the result of a callback into a vector.
 */
template <typename Callable = std::identity>
    requires std::regular_invocable<Callable, std::string&>
class line_parser {
public:
    line_parser(Callable callback = {})
        : m_callback(callback)
    {}

    template <typename T = std::remove_cvref_t<std::invoke_result_t<Callable, std::string&>>>
    [[nodiscard]] auto operator()(std::istream& inf) {
        auto ret = std::vector<T>();
        for (std::string line; std::getline(inf, line); ) {
            ret.push_back(m_callback(line));
        }
        return ret;
    }

private:
    Callable m_callback;
};

/**
 * @brief   Read a file and process its content with the given `Parser`.
 */
template <typename Parser = detail::def_parser>
[[nodiscard]] auto read_file(const std::string& path, Parser parser = {})
        -> expected<std::remove_cvref_t<std::invoke_result_t<Parser, std::istream&>>, error>
{
    const auto fs = detail::fs_path(path);
    if (not fs.has_value()) {
        return { unexpect, fs.error() };
    }

    auto stream = std::ifstream(*fs);
    return parser(stream);
}

template <typename Parser = detail::def_bin_parser>
[[nodiscard]] auto read_bin(const std::string& path, Parser parser = {})
        -> expected<std::remove_cvref_t<std::invoke_result_t<Parser, std::istream&>>, error>
{
    const auto fs = detail::fs_path(path);
    if (not fs.has_value()) {
        return { unexpect, fs.error() };
    }

    auto stream = std::ifstream(*fs, std::ios::binary);
    return parser(stream);
}


} // namespace nova
