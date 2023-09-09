#include <gmock/gmock.h>

#include "nova/utils.h"

TEST(Utils, ph) {
    EXPECT_EQ(nova::ph(), 1);
}
