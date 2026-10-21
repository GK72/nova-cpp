#include <libnova/error.hpp>

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <source_location>

const auto FuncThrowLine = std::source_location::current().line() + 3;

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

        EXPECT_EQ(nex.where().line(), FuncThrowLine);
        EXPECT_THAT(nex.where().function_name(), testing::HasSubstr("func"));

#ifdef NOVA_EXPERIMENTAL_FEATURE_SET
        EXPECT_THAT(
            fmt::format("{}", nex.backtrace()),
            testing::HasSubstr("error.test.cpp")
        );
#else
        EXPECT_THAT(
            fmt::format("{}", nex.backtrace()),
            testing::HasSubstr("Backtrace: Not supported")
        );
#endif
    }
}

TEST(Error, Exception_FmtString) {
    const auto msg = "some error";
    const auto line = std::source_location::current().line() + 1;
    const auto ex = nova::exception("An error: {}", msg);

    EXPECT_STREQ(ex.what(), "An error: some error");
    EXPECT_EQ(ex.where().line(), line);
    EXPECT_THAT(ex.where().file_name(), testing::HasSubstr("error.test.cpp"));
}
