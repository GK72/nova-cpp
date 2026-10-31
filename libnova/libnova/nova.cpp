#include <libnova/detail/version.hpp>

#include <fmt/format.h>

#include <string>

namespace nova {

auto library_version() -> std::string {
    return fmt::format("v{}.{}.{}", NovaVersionMajor, NovaVersionMinor, NovaVersionPatch);
}

} // namespace nova
