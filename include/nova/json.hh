/**
 * Part of Nova C++ Library.
 *
 * JSON API for uniform document handling.
 */

#pragma once

#include "nova/error.hh"

#include <fmt/format.h>

#define JSON_DIAGNOSTICS 1

#ifdef NOVA_GCC
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=106247
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#include <nlohmann/json.hpp>
#pragma GCC diagnostic pop
#else
#include <nlohmann/json.hpp>
#endif

#include <algorithm>
#include <concepts>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace nova {

/**
 * @brief   JQ-like path expression.
 *
 * DOM = Document Object Model
 */
class dom_path {
public:
    dom_path(const std::string& path)
        : m_path(path)
    {}

    /**
     * @brief   Convert path to RFC 6901.
     *
     * https://datatracker.ietf.org/doc/html/rfc6901
     */
    [[nodiscard]] std::string rfc6901() {
        if (m_path.empty()) {
            return {};
        }

        std::ranges::replace(m_path, '.', '/');
        return fmt::format("/{}", m_path);
    }

private:
    std::string m_path;

};

class json {
public:
    [[nodiscard]] explicit json(const std::string& content) try
        : m_data(nlohmann::json::parse(content))
    {}
    catch (const nlohmann::json::exception& ex) {
        throw parsing_error(ex.what());
    }

    [[nodiscard]] std::string dump(int indent = -1) const {
        return m_data.dump(indent);
    }

    template <typename R> requires std::is_fundamental_v<R>
    [[nodiscard]] R lookup(const std::string& path) const {
        return m_data.at(make_json_pointer(path)).template get<R>();
    }

    template <typename R>
    [[nodiscard]] const R& lookup(const std::string& path) const {
        return m_data.at(make_json_pointer(path)).template get_ref<const R&>();
    }

    template <typename R> requires std::is_fundamental_v<R>
    [[nodiscard]] R lookup(const std::string& path, const R& def) const {
        if (not contains(path)) {
            return def;
        }
        return lookup<R>(path);
    }

    template <typename R>
    [[nodiscard]] const R& lookup(const std::string& path, const R& def) const {
        if (not contains(path)) {
            return def;
        }
        return lookup<R>(path);
    }

    /**
     * @brief   Return a JSON object without leaking the underlying API.
     */
    [[nodiscard]] auto at(const std::string& path) const {
        auto doc = json();
        doc.set(m_data.at(make_json_pointer(path)));
        return doc;
    }

    [[nodiscard]] bool contains(const std::string& path) const {
        return m_data.contains(make_json_pointer(path));
    }

private:
    nlohmann::json m_data;

    /**
     * @brief   Hidden default constructor to overcome ambiguity.
     */
    json() = default;

    /**
     * @brief   Implicit conversions make the constructor ambiguous.
     */
    void set(const nlohmann::json& json_object) {
        m_data = json_object;
    }

    [[nodiscard]] static auto make_json_pointer(const std::string& path)
            -> nlohmann::json::json_pointer
    {
        return nlohmann::json::json_pointer(dom_path(path).rfc6901());
    }
};

} // namespace nova
