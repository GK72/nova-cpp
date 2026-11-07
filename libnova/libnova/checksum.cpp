/**
 * Part of Nova C++ Library.
 *
 * @brief   Checksums.
 *
 * The implementations live in `detail/`, see `detail/crc32.hpp`.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-07
 * @updated     2026-11-07
 * @status      evolving
 * SPDX-License-Identifier: BSL-1.0
 */

#include <libnova/checksum.hpp>
#include <libnova/detail/crc32.hpp>

#include <cstdint>

namespace nova {

auto crc32(nova::data_view data, std::uint32_t crc) -> std::uint32_t {
    return ~detail::crc32_update(data.span(), ~crc);
}

} // namespace nova
