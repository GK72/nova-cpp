#include <gmock/gmock.h>

#include "nova/utils.h"

#include <array>

TEST(Utils, Concat) {
    EXPECT_EQ(
        nova::concat(
            std::to_array({ 4, 6, 8 }),
            std::to_array({ 1, 7, 2 }),
            std::to_array({ 2, 3, 1 })
        ),
        std::to_array({
            4, 6, 8,
            1, 7, 2,
            2, 3, 1
        })
    );
}
