/**
 * Part of Nova C++ Library.
 *
 * YAML API.
 */

#include "nova/error.h"

#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace nova {

// TODO(refact): generalize split and move into `utils.h`
std::vector<std::string> split(const std::string &str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        if (not token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

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

    [[nodiscard]] std::vector<yaml> lookup_vec(std::string_view path) const {
        const auto node = lookup_impl(path);
        return std::vector<yaml>(node.begin(), node.end());
    }

    [[nodiscard]] std::map<std::string, yaml> lookup_map(std::string_view path) const {
        std::map<std::string, yaml> ret;
        const auto node = lookup_impl(path);

        for (const auto& elem : node) {
            ret.insert({ elem.first.as<std::string>(), yaml{ elem.second } });
        }

        return ret;
    }

private:
    YAML::Node m_doc;

    YAML::Node lookup_impl(std::string_view path) const {
        try {
            std::vector<std::string> keys = split(std::string(path), '.');
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
