/**
 * Part of Nova C++ Library
 *
 * @brief   Dynamically typed value trees.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-10-31
 * @updated     2026-10-31
 * @status      Prototype
 */

#include <libnova/dyn_value.hpp>

#include <libnova/detail/json.hpp>
#include <libnova/detail/toml.hpp>
#include <libnova/detail/yaml.hpp>
#include <libnova/io.hpp>

#include <fmt/format.h>

#include <array>
#include <charconv>
#include <fstream>
#include <system_error>

namespace nova {

namespace {

    /**
     * @brief   `segment` as a sequence index, if all of it is a number.
     */
    [[nodiscard]] auto as_index(std::string_view segment) -> std::optional<std::size_t> {
        auto index = std::size_t{ };
        const auto [end, ec] = std::from_chars(segment.data(), segment.data() + segment.size(), index);

        if (segment.empty() or ec != std::errc{ } or end != segment.data() + segment.size()) {
            return std::nullopt;
        }

        return index;
    }

    /**
     * @brief   `node` without nulls in its maps, at any depth.
     */
    [[nodiscard]] auto without_nulls(dyn_value node) -> dyn_value {
        if (const auto* seq = std::get_if<dyn_value::seq_t>(&node.data())) {
            auto ret = dyn_value::seq_t{ };

            for (const auto& item : *seq) {
                ret.push_back(without_nulls(item));
            }

            return ret;
        }

        const auto* map = std::get_if<dyn_value::map_t>(&node.data());
        if (map == nullptr) {
            return node;
        }

        auto ret = dyn_value::map_t{ };
        for (const auto& [key, item] : *map) {
            if (not item.is_null()) {
                ret.emplace(key, without_nulls(item));
            }
        }

        return ret;
    }

    /**
     * @brief   Puts `path` in front of the detail of an error, for one about the
     *          content of that file.
     */
    [[nodiscard]] auto in_file(const std::filesystem::path& path) {
        return [path](error err) {
            err.detail = fmt::format("{}: {}", path.string(), err.detail);
            return err;
        };
    }

    [[nodiscard]] auto create_parent(const std::filesystem::path& path) -> expected<empty, error> {
        auto ec = std::error_code{ };
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path(), ec);
        }

        return ec
            ? expected<empty, error>{
                wrap_error(ec, path.parent_path().string())
            }
            : empty{ };
    }

    [[nodiscard]] auto write_file(const std::filesystem::path& path, const std::string& text) -> expected<empty, error> {
        auto out = std::ofstream(path, std::ios::binary);
        out << text;
        out.close();

        return out
            ? expected<empty, error>{ empty{ } }
            : wrap_error(std::errc::io_error, path.string());
    }

    [[nodiscard]] auto rename_file(const std::filesystem::path& from, const std::filesystem::path& to) -> expected<empty, error> {
        auto ec = std::error_code{ };
        std::filesystem::rename(from, to, ec);

        return ec
            ? expected<empty, error>{ wrap_error(ec, to.string()) }
            : empty{ };
    }

    /**
     * @brief   Replace the file at `path` with `text` at once, creating its
     *          directory: a failed write leaves the old file in place.
     */
    [[nodiscard]] auto replace_file(const std::filesystem::path& path, const std::string& text) -> expected<empty, error> {
        auto temp = path;
        temp += ".tmp";

        return create_parent(path)
            .and_then([&temp, &text](empty /* done */) { return write_file(temp, text); })
            .and_then([&temp, &path](empty /* done */) { return rename_file(temp, path); });
    }

} // namespace

// --- dyn_path ---

dyn_path::dyn_path(std::string_view dotted) {
    if (dotted.empty()) {
        return;
    }

    auto segment = std::string{ };

    for (std::size_t pos = 0; pos < dotted.size(); ++pos) {
        const auto chr = dotted[pos];

        if (chr == '\\' and pos + 1 < dotted.size()) {
            segment += dotted[++pos];
        } else if (chr == '.') {
            m_segments.push_back(std::move(segment));
            segment.clear();
        } else {
            segment += chr;
        }
    }

    m_segments.push_back(std::move(segment));
}

dyn_path::dyn_path(std::initializer_list<std::string_view> segments)
    : m_segments(segments.begin(), segments.end())
{}

auto dyn_path::operator/(std::string_view key) const -> dyn_path {
    auto ret = *this;
    ret.m_segments.emplace_back(key);
    return ret;
}

auto dyn_path::operator/(std::size_t index) const -> dyn_path {
    auto ret = *this;
    ret.m_segments.push_back(std::to_string(index));
    return ret;
}

auto dyn_path::operator/(const dyn_path& tail) const -> dyn_path {
    auto ret = *this;
    ret.m_segments.insert(ret.m_segments.end(), tail.m_segments.begin(), tail.m_segments.end());
    return ret;
}

auto dyn_path::prefix(std::size_t count) const -> dyn_path {
    auto ret = dyn_path{ };
    ret.m_segments.assign(m_segments.begin(), m_segments.begin() + static_cast<std::ptrdiff_t>(count));
    return ret;
}

auto dyn_path::to_string() const -> std::string {
    if (m_segments.empty()) {
        return "<root>";
    }

    auto ret = std::string{ };

    for (const auto& segment : m_segments) {
        if (not ret.empty()) {
            ret += '.';
        }

        for (const auto chr : segment) {
            if (chr == '.' or chr == '\\') {
                ret += '\\';
            }
            ret += chr;
        }
    }

    return ret;
}

// --- dyn_value ---

dyn_value::dyn_value(float number)
    : m_data(static_cast<double>(number))
{
    auto buffer = std::array<char, 32>{ };
    const auto [end, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), number);
    if (ec == std::errc{ }) {
        std::from_chars(buffer.data(), end, std::get<double>(m_data));
    }
}

auto dyn_value::seq(std::initializer_list<dyn_value> items) -> dyn_value {
    return seq_t(items.begin(), items.end());
}

auto dyn_value::map(std::initializer_list<std::pair<const std::string, dyn_value>> items) -> dyn_value {
    return map_t(items.begin(), items.end());
}

auto dyn_value::walk(const dyn_path& path) const -> expected<const dyn_value*, dyn_mismatch> {
    const auto* node = this;
    const auto& segments = path.segments();

    for (std::size_t depth = 0; depth < segments.size(); ++depth) {
        if (node->is_null()) {
            return static_cast<const dyn_value*>(nullptr);
        }

        const auto& segment = segments[depth];

        if (const auto* map = std::get_if<map_t>(&node->m_data)) {
            const auto found = map->find(segment);
            if (found == map->end()) {
                return static_cast<const dyn_value*>(nullptr);
            }
            node = &found->second;
        }
        else if (const auto* seq = std::get_if<seq_t>(&node->m_data); seq != nullptr and as_index(segment)) {
            const auto index = *as_index(segment);
            if (index >= seq->size()) {
                return static_cast<const dyn_value*>(nullptr);
            }
            node = &(*seq)[index];
        }
        else {
            return unexpected<dyn_mismatch>{ {
                .where = path.prefix(depth),
                .expected = seq != nullptr ? "an index" : "map or seq",
                .got = seq != nullptr ? fmt::format("key `{}`", segment) : std::string(node->type_name()),
            } };
        }
    }

    return node->is_null() ? nullptr : node;
}

void dyn_value::throw_mismatch(const dyn_path& base, const dyn_mismatch& mismatch) {
    throw exception("{}", dyn_mismatch{
        .where = base / mismatch.where,
        .expected = mismatch.expected,
        .got = mismatch.got,
    });
}

auto dyn_value::at(const dyn_path& path) const -> const dyn_value& {
    const auto node = walk(path);

    if (not node) {
        throw_mismatch({ }, node.error());
    }

    if (*node == nullptr) {
        throw exception("Missing `{}`", path);
    }

    return **node;
}

auto dyn_value::as_map() const -> const map_t& {
    if (const auto* map = std::get_if<map_t>(&m_data)) {
        return *map;
    }

    throw_mismatch({ }, { .where = { }, .expected = "map", .got = std::string(type_name()) });
}

auto dyn_value::as_seq() const -> const seq_t& {
    if (const auto* seq = std::get_if<seq_t>(&m_data)) {
        return *seq;
    }

    throw_mismatch({ }, { .where = { }, .expected = "seq", .got = std::string(type_name()) });
}

auto dyn_value::type_name() const -> std::string_view {
    return std::visit(
        []<typename T>(const T&) -> std::string_view {
            if constexpr (std::same_as<T, std::monostate>) {
                return "null";
            } else if constexpr (std::same_as<T, bool>) {
                return "bool";
            } else if constexpr (std::same_as<T, std::int64_t>) {
                return "int";
            } else if constexpr (std::same_as<T, double>) {
                return "float";
            } else if constexpr (std::same_as<T, std::string>) {
                return "string";
            } else if constexpr (std::same_as<T, seq_t>) {
                return "seq";
            } else {
                return "map";
            }
        }
        , m_data
    );
}

auto dyn_value::merge(dyn_value base, const dyn_value& overlay) -> dyn_value {
    auto* base_map = std::get_if<map_t>(&base.m_data);
    const auto* overlay_map = std::get_if<map_t>(&overlay.m_data);

    if (base_map == nullptr or overlay_map == nullptr) {
        return without_nulls(overlay);
    }

    for (const auto& [key, item] : *overlay_map) {
        const auto found = base_map->find(key);
        if (item.is_null()) {
            if (found != base_map->end()) {
                base_map->erase(found);
            }
        } else if (found != base_map->end()) {
            found->second = merge(std::move(found->second), item);
        } else {
            base_map->emplace(key, without_nulls(item));
        }
    }

    return base;
}

auto dyn_value::format_of(const std::filesystem::path& path) -> expected<format_e, error> {
    const auto extension = path.extension().string();

    if (extension == ".yaml" or extension == ".yml") {
        return format_e::yaml;
    }

    if (extension == ".json") {
        return format_e::json;
    }

    if (extension == ".toml") {
        return format_e::toml;
    }

    return wrap_error(error_e::unknown_format, path.string());
}

auto dyn_value::parse(std::string_view text, format_e format) -> expected<dyn_value, error> {
    if (text.find_first_not_of(" \t\r\n") == std::string_view::npos) {
        return dyn_value{ };
    }

    switch (format) {
        case format_e::yaml: return detail::yaml::parse(text);
        case format_e::json: return detail::json::parse(text);
        case format_e::toml: return detail::toml::parse(text);
    }

    return wrap_error(error_e::unknown_format);
}

auto dyn_value::read_from(const std::filesystem::path& path) -> expected<dyn_value, error> {
    return format_of(path).and_then([&path](format_e format) {
        return read_file(path.string()).and_then([&path, format](const std::string& text) {
            return parse(text, format).transform_error(in_file(path));
        });
    });
}

auto dyn_value::as_config(dyn_value doc) -> expected<dyn_value, error> {
    if (doc.is_null()) {
        return dyn_value{ map_t{ } };
    }

    if (not std::holds_alternative<map_t>(doc.data())) {
        return wrap_error(error_e::not_a_map, std::string(doc.type_name()));
    }

    return doc;
}

auto dyn_value::read_config(const std::filesystem::path& path) -> expected<dyn_value, error> {
    return read_from(path).and_then([&path](dyn_value doc) {
        return as_config(std::move(doc)).transform_error(in_file(path));
    });
}

auto dyn_value::dump(format_e format) const -> expected<std::string, error> {
    switch (format) {
        case format_e::yaml: return detail::yaml::format(*this);
        case format_e::json: return detail::json::format(*this);
        case format_e::toml: return detail::toml::format(*this);
    }

    return wrap_error(error_e::unknown_format);
}

auto dyn_value::write_to(const std::filesystem::path& path) const -> expected<empty, error> {
    return format_of(path)
        .and_then([this, &path](format_e format) {
            return dump(format).transform_error(in_file(path));
        })
        .and_then([&path](const std::string& text) {
            return replace_file(path, text);
        });
}

} // namespace nova

auto fmt::formatter<nova::dyn_value>::format(const nova::dyn_value& node, format_context& ctx) const
        -> format_context::iterator
{
    const auto text = std::visit(
        [&node]<typename T>(const T& contained) -> std::string {
            if constexpr (std::same_as<T, bool>) {
                return contained ? "true" : "false";
            } else if constexpr (std::same_as<T, std::int64_t> or std::same_as<T, double>) {
                return fmt::format("{}", contained);
            } else if constexpr (std::same_as<T, std::string>) {
                return contained;
            } else {
                throw nova::exception("Cannot format a {} as text", node.type_name());
            }
        }
        , node.data()
    );

    return fmt::formatter<std::string_view>::format(text, ctx);
}
