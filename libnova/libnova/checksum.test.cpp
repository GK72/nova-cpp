/**
 * Part of Nova C++ Library.
 *
 * @brief   Tests of `checksum.hpp`.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-10-24
 * @updated     2026-11-07
 * @reviewed    read
 * SPDX-License-Identifier: BSL-1.0
 */

#include <libnova/checksum.hpp>
#include <libnova/detail/crc32.hpp>
#include <libnova/data.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

using namespace nova::literals;

TEST(Checksum, Crc32_CheckValue) {
    EXPECT_EQ(nova::crc32("123456789"_data), 0xCBF43926u);
    EXPECT_EQ(nova::crc32(""_data), 0u);
    EXPECT_EQ(nova::crc32("The quick brown fox jumps over the lazy dog"_data), 0x414FA339u);
}

TEST(Checksum, Crc32_InPieces) {
    EXPECT_EQ(nova::crc32("6789"_data, nova::crc32("12345"_data)), 0xCBF43926u);
}

TEST(Checksum, Crc32_BackendsMatchBytewise) {
    std::vector<std::byte> data(1000);
    for (std::size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<std::byte>((i * 151U) ^ (i >> 3U));
    }
    const auto all = std::span<const std::byte>(data);

    for (const auto& backend : nova::detail::crc32_backends()) {
        // Every length across the steps of every implementation (up to 256
        // bytes, twice over), from every offset within a 16-byte block
        std::size_t mismatches = 0;
        for (std::size_t offset = 0; offset < 16; ++offset) {
            for (std::size_t length = 0; length <= 600; ++length) {
                const auto piece = all.subspan(offset, length);
                if (backend.fn(piece, 0xFFFFFFFFU) != nova::detail::crc32_bytewise(piece, 0xFFFFFFFFU)) {
                    ADD_FAILURE() << backend.name << ": offset " << offset << ", length " << length;
                    if (++mismatches == 5) {
                        break;
                    }
                }
            }
        }

        // Continuing from a register that is not the initial one
        EXPECT_EQ(backend.fn(all, 0x12345678U), nova::detail::crc32_bytewise(all, 0x12345678U)) << backend.name;
    }
}

TEST(Checksum, Crc32_BackendsInclude) {
    std::vector<std::string_view> names;
    for (const auto& backend : nova::detail::crc32_backends()) {
        names.push_back(backend.name);
    }
    // Printed, to see in the log which ones ran
    std::cout << "CRC-32 implementations:";
    for (const auto name : names) {
        std::cout << ' ' << name;
    }
    std::cout << '\n';
    EXPECT_GE(names.size(), 2U);
}
