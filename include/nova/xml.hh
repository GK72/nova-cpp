/**
 * Part of Nova C++ Library.
 *
 * XML API.
 */

#include <libxml++/document.h>
#include <libxml++/libxml++.h>

#include <filesystem>

namespace nova {

class xml {
    auto pre_init(const std::string& content) {
        m_parser.parse_memory(content);

        return m_parser.get_document();
    }

public:
    explicit xml(const std::string& content)
        : m_doc(pre_init(content))
    {}

    explicit xml(const char* content)
        : m_doc(pre_init(content))
    {}

    explicit xml(const std::filesystem::path& path)
        : m_parser(path.string())
        , m_doc(m_parser.get_document())
    {}

    auto string() const {
        return m_doc->write_to_string();
    }

private:
    xmlpp::DomParser m_parser;
    std::unique_ptr<xmlpp::Document> m_doc;
};

} // namespace nova
