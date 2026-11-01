#pragma once

#include <string_view>

namespace nova {

/**
 * @brief   Version of the linked Nova library, e.g. `0.10.0`.
 */
[[nodiscard]] auto library_version() -> std::string_view;

/**
 * @brief   Version and the git state Nova was built from.
 *
 * Format: `<major>.<minor>.<patch>+<hash>.<branch>[.dirty]`, e.g.
 * `0.10.0+a96c838.main`. Unknown git parts are `unknown`.
 */
[[nodiscard]] auto library_full_version() -> std::string_view;

} // namespace nova
