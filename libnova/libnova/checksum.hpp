/**
 * Part of Nova C++ Library.
 *
 * @brief   Checksums.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-10-24
 * @updated     2026-11-07
 * @status      evolving
 * @reviewed    skimmed
 * SPDX-License-Identifier: BSL-1.0
 */

#pragma once

#include <libnova/data.hpp>

#include <cstdint>

namespace nova {

/**
 * @brief   CRC-32 (IEEE 802.3, as used by PNG and zlib).
 *
 * The checksum is the remainder of a polynomial division. The data is one long
 * binary polynomial (each bit a coefficient), divided by the generator
 * polynomial 0x04C11DB7 in carry-less arithmetic, where subtraction is XOR.
 * Flipping any bit of the data changes the 32-bit remainder, and every burst of
 * errors up to 32 bits long is caught.
 *
 * The division runs in a shift register. Bits enter least significant first, so
 * the register shifts right and holds the generator bit-reversed (0xEDB88320):
 * when the bit shifted out is 1, the generator is XOR-ed in. A table holds the
 * result of eight such steps for each value of the low byte, so a byte costs one
 * lookup instead of eight steps. The faster implementations, chosen by what the
 * CPU supports, are explained in the SIMD Guide (`doc/simd-guide.adoc#crc32`).
 *
 * The register starts with all ones, so leading zero bytes change the result,
 * and the result is inverted. Inverting `crc` on entry undoes that, so a call
 * continues where the previous one stopped, and the default 0 starts afresh.
 *
 * @param   crc     result of the previous call, to checksum data in pieces
 */
[[nodiscard]] auto crc32(nova::data_view data, std::uint32_t crc = 0) -> std::uint32_t;

} // namespace nova
