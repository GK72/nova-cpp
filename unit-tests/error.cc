#include "nova/error.hh"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fmt/format.h>

void func() {
    THROWUP;
}

void func2() {
    throw nova::exception("An error", nova::error{ "Some data" });
}

TEST(Error, ExceptionVoid) {
    try {
        func();
        FAIL() << "Expected an exception to be thrown!";
    } catch (const std::exception& ex) {
        EXPECT_STREQ(ex.what(), "ERROAR");

        const auto& nex = dynamic_cast<const nova::exception<void>&>(ex);
        EXPECT_THAT(
            fmt::format("{}", nex.where()),
            testing::HasSubstr(
                "nova/unit-tests/error.cc:8, from function `void func()`"
            )
        );

#ifdef NOVA_EXPERIMENTAL_FEATURE_SET
        EXPECT_THAT(
            fmt::format("{}", nex.backtrace()),
            testing::HasSubstr("error.cc")
        );
#endif
    }
}

TEST(Error, ExceptionWithData) {
    try {
        func2();
        FAIL() << "Expected an exception to be thrown!";
    } catch (const nova::exception<nova::error>& ex) {
        EXPECT_STREQ(ex.what(), "An error");

        EXPECT_THAT(
            fmt::format("{}", ex.where()),
            testing::HasSubstr(
                "nova/unit-tests/error.cc:12, from function `void func2()`"
            )
        );
    }
}
