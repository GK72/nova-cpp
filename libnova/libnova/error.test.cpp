#include <libnova/error.hpp>

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

#ifdef NOVA_EXPERIMENTAL_FEATURE_SET
        EXPECT_THAT(
            fmt::format("{}", nex.where()),
            testing::HasSubstr(
                "unit-tests/error.cc:8, from function `void func()`"
            )
        );

        EXPECT_THAT(
            fmt::format("{}", nex.backtrace()),
            testing::HasSubstr("error.cc")
        );
#else
        EXPECT_THAT(
            fmt::format("{}", nex.where()),
            testing::HasSubstr("Source location: Not supported")
        );

        EXPECT_THAT(
            fmt::format("{}", nex.backtrace()),
            testing::HasSubstr("Backtrace: Not supported")
        );
#endif
    }
}

TEST(Error, Exception_FmtString) {
    const auto msg = "some error";
    const auto ex = nova::exception("An error: {}", msg);
    EXPECT_STREQ(ex.what(), "An error: some error");
}

TEST(Error, MessageOnly) {
    const auto err = nova::error("Something failed");

    EXPECT_EQ(err.message, "Something failed");
    EXPECT_TRUE(err.detail.empty());
    EXPECT_TRUE(err.category.empty());
    EXPECT_EQ(fmt::format("{}", err), "Something failed");
}

TEST(Error, CategoryAndDetail) {
    const auto err = nova::error("io", "Cannot read the file", "/etc/missing");

    EXPECT_EQ(err.category, "io");
    EXPECT_EQ(err.message, "Cannot read the file");
    EXPECT_EQ(err.detail, "/etc/missing");
    EXPECT_EQ(fmt::format("{}", err), "[io] Cannot read the file (/etc/missing)");
    EXPECT_EQ(fmt::format("{}", nova::error("parse", "Not a number")), "[parse] Not a number");
}

TEST(Error, ComparedByAllParts) {
    EXPECT_EQ(nova::error("io", "Cannot read", "a"), nova::error("io", "Cannot read", "a"));
    EXPECT_NE(nova::error("io", "Cannot read", "a"), nova::error("io", "Cannot read", "b"));
}
