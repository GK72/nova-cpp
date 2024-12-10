/**
 * Part of Nova C++ Library.
 *
 * YAML API.
 */
#pragma once

#include "nova/error.hh"
#include "nova/type_traits.hh"
#include "nova/utils.hh"

#include <yaml-cpp/node/emit.h>
#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace nova {

class yaml {
public:
    explicit yaml(const std::string& content)
        : m_doc(YAML::Load(content))
    {}

    explicit yaml(const char* content)
        : m_doc(YAML::Load(content))
    {}

    explicit yaml(const std::filesystem::path& content)
        : m_doc(YAML::LoadFile(content.string()))
    {}

    explicit yaml(const YAML::Node& doc)
        : m_doc(YAML::Clone(doc))
    {}

    [[nodiscard]] std::string dump() const {
        return YAML::Dump(m_doc);
    }

    template <typename T>
    [[nodiscard]] T as() const {
        try {
            return m_doc.as<T>();
        } catch (const YAML::BadConversion& ex) {
            throw nova::parsing_error(ex.what());
        }
    }

    template <typename T>
    [[nodiscard]] T lookup(std::string_view path) const {
        try {
            return lookup_impl(path).as<T>();
        } catch (const YAML::BadConversion& ex) {
            throw nova::parsing_error(ex.what());
        }
    }

    template <template <typename...> typename Container>
        requires vector_like<Container<yaml>>
    [[nodiscard]] Container<yaml> lookup(std::string_view path) const {
        const auto node = lookup_impl(path);
        return Container<yaml>(node.begin(), node.end());
    }

    template <template <typename, typename...> typename Container>
        requires map_like<Container<std::string, yaml>>
    [[nodiscard]] Container<std::string, yaml> lookup(std::string_view path) const {
        Container<std::string, yaml> ret;
        const auto node = lookup_impl(path);

        for (const auto& elem : node) {
            ret.insert({ elem.first.as<std::string>(), yaml{ elem.second } });
        }

        return ret;
    }

    /**
     * @brief   Return a YAML object without leaking the underlying API.
     */
    [[nodiscard]] auto at(const std::string& path) const {
        auto doc = yaml();
        doc.set(lookup_impl(path));
        return doc;
    }

private:
    YAML::Node m_doc;

    /**
     * @brief   Hidden default constructor to overcome ambiguity.
     */
    yaml() = default;

    /**
     * @brief   Implicit conversions make the constructor ambiguous.
     */
    void set(const YAML::Node& yaml_object) {
        m_doc = yaml_object;
    }

    YAML::Node lookup_impl(std::string_view path) const {
        try {
            auto temp = split(path, ".")
                      | std::views::filter([](const auto& elem) { return std::size(elem) > 0; });
            const auto keys = std::vector<std::string>(std::begin(temp), std::end(temp));
            YAML::Node node = YAML::Clone(m_doc);

            for (const std::string& key : keys) {
                if (node.IsMap()) {
                    node = node[key];
                } else if (node.IsSequence()) {
                    node = node[std::stoi(key)];
                }
            }

            return node;
        } catch (const std::exception& ex) {
            throw nova::parsing_error(ex.what());
        }
    }

};

} // namespace nova
