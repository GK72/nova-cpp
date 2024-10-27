/**
 * Part of Nova C++ Library.
 *
 * Input/Output related functionalities.
 */

#pragma once

#include "nova/error.hh"
#include "nova/utils.hh"

#include <fmt/format.h>

#include <functional>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

namespace nova {

namespace detail {

    /**
     * @brief   Default parser which returns the whole file as one string.
     */
    struct def_parser {
        [[nodiscard]] std::string operator()(std::istream& inf) {
            std::stringstream ss;
            for (std::string line; std::getline(inf, line); ) {
                ss << line << '\n';
            }
            return ss.str();
        }
    };
}

/**
 * @brief   A parser that puts the result of a callback into a vector.
 */
template <typename Callable = std::identity>
class line_parser {
public:
    line_parser(Callable&& callback = {})
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
[[nodiscard]] auto read_file(const std::string& path, Parser&& parser = {})
        -> expected<std::remove_cvref_t<std::invoke_result_t<Parser, std::istream&>>, error>
{
    const auto fs = std::filesystem::path(path);
    if (not std::filesystem::is_regular_file(fs)) {
        return unexpected<error>{ fmt::format("{} is not a regular file!", std::filesystem::absolute(fs).string()) };
    }

    auto stream = std::ifstream(fs);
    return parser(stream);
}

} // namespace nova
