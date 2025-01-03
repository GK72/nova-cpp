/**
 * Nova test utilities.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "nova/error.hh"

#define EXPECT_ASSERTION_FAIL(expr)                                            \
    EXPECT_THAT(                                                               \
        [](){ expr; },                                                         \
        testing::ThrowsMessage<nova::exception>(testing::StartsWith("Assertion failed: ")))
