/**
 * Nova test utilities.
 */

#include "nova/error.hh"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#define EXPECT_ASSERTION_FAIL(expr)                                            \
    EXPECT_THAT(                                                               \
        [](){ expr; },                                                         \
        testing::ThrowsMessage<nova::exception>(testing::StartsWith("Assertion failed: ")))
