#include <libnova/version.hpp>

#include <nova/gen/build_info.hpp>

namespace nova {

auto library_version() -> std::string_view {
    return detail::version();
}

auto library_full_version() -> std::string_view {
    return detail::full_version();
}

} // namespace nova
