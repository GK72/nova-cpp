#include "nova/error.hh"

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

void func() {
    THROWUP;
}

TEST(Error, Exception) {
    try {
        func();
        FAIL() << "Expected an exception to be thrown!";
    } catch (const std::exception& ex) {
        EXPECT_STREQ(ex.what(), "ERROAR");

        const auto& nex = dynamic_cast<const nova::exception&>(ex);
        EXPECT_THAT(
            fmt::format("{}", nex.where()),
            testing::HasSubstr(
                "unit-tests/error.cc:8, from function `void func()`"
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

TEST(Error, Exception_FmtString) {
    const auto msg = "some error";
    const auto ex = nova::exception("An error: {}", msg);
    EXPECT_STREQ(ex.what(), "An error: some error");
}
