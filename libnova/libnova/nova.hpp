/**
 * Part of Nova C++ Library.
 *
 * Main wrapper header.
 */

#pragma once

#include <libnova/details/version.hpp>

#include <libnova/color.hpp>
#include <libnova/data.hpp>
#include <libnova/error.hpp>
#include <libnova/expected.hpp>
#include <libnova/flat_map.hpp>
#include <libnova/intrinsics.hpp>
#include <libnova/io.hpp>
#include <libnova/json.hpp>
#include <libnova/log.hpp>
#include <libnova/main.hpp>
#include <libnova/not_null.hpp>
#include <libnova/parse.hpp>
#include <libnova/random.hpp>
#include <libnova/static_string.hpp>
#include <libnova/std_extensions.hpp>
#include <libnova/system.hpp>
#include <libnova/threading.hpp>
#include <libnova/type_traits.hpp>
#include <libnova/types.hpp>
#include <libnova/units.hpp>
#include <libnova/utils.hpp>
#include <libnova/vec.hpp>
#include <libnova/yaml.hpp>

#include <string>

namespace nova {

[[nodiscard]] auto library_version() -> std::string;

} // namespace nova
