#include "test_utils.hh"

#include "nova/expected.hh"

#include <fmt/format.h>
#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>
#include <tuple>

struct custom_error {
    // P0960R3 is not implemented in Apple Clang
    constexpr custom_error(int x)
        : code(x)
    {}

    int code;
};

template <>
class fmt::formatter<custom_error> {
public:
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template <typename FmtContext>
    auto format(custom_error err, FmtContext& ctx) const {
        return fmt::format_to(ctx.out(), "The number {}", err.code);
    }
};

TEST(Expected, TypeTraits) {
    using E = nova::expected<int, std::string_view>;
    static_assert(nova::is_expected_v<E>);
    static_assert(not nova::is_expected_v<int>);
}

TEST(Expected, TrivialTypes) {
    constexpr auto expect = [](bool condition) -> nova::expected<int, std::string_view> {
        if (condition) {
            return 1;
        }
        return { nova::unexpect, "Error message" };
    };

    {
        constexpr auto ret = expect(true);

        EXPECT_TRUE(ret);
        EXPECT_EQ(ret.value(), 1);
        EXPECT_EQ(*ret, 1);
    }

    {
        constexpr auto ret = expect(false);

        using namespace std::literals::string_view_literals;

        EXPECT_TRUE(not ret);
        EXPECT_EQ(ret.error(), "Error message"sv);
    }
}

TEST(Expected, Value_SafeAccess) {
    constexpr auto x = nova::expected<int, int>(nova::unexpect, 8);
    EXPECT_THROW(std::ignore = x.value(), nova::exception);

    EXPECT_THROWN_MESSAGE(
        ( std::ignore = nova::expected<int, int>(nova::unexpect, 8).value() ),
        "8"
    );
}

TEST(Expected, Error_SafeAccess) {
    constexpr auto x = nova::expected<int, int>(8);
    EXPECT_THROWN_MESSAGE(std::ignore = x.error(), "Bad expected access: it has no error");

    EXPECT_THROWN_MESSAGE(
        ( std::ignore = nova::expected<int, int>(8).error() ),
        "Bad expected access: it has no error"
    );
}

TEST(Expected, CompoundErrorTypeMessage) {
    constexpr auto x = nova::expected<int, custom_error>(nova::unexpect, 8);
    EXPECT_THROWN_MESSAGE(std::ignore = x.value(), "Bad expected access: The number 8");
}

TEST(Expected, NonTrivialTypes) {
    const auto x = nova::expected<nt, int>(nt{ });
    EXPECT_TRUE(x);
}

TEST(Expected, NonTrivialDestructor_LeakSanyiOn) {
    const auto x = nova::expected<std::unique_ptr<int>, int>(std::make_unique<int>(9));
    EXPECT_TRUE(x);
    // Leak sanitizer fires if the destructor of the implementation (`vex`) is empty.
}

TEST(Expected, SameTypes) {
    constexpr auto x = nova::expected<int, int>(9);
    ASSERT_TRUE(x);
    EXPECT_EQ(*x, 9);

    constexpr auto y = nova::expected<int, int>(nova::unexpect, 8);
    ASSERT_TRUE(not y);
    EXPECT_EQ(y.error(), 8);
}

TEST(Expected, ValueConversion) {
    constexpr auto x = nova::expected<std::string_view, int>("hello");
    EXPECT_EQ(x.value(), "hello");
}

TEST(Expected, ErrorConversion) {
    constexpr auto x = nova::expected<int, std::string_view>(nova::unexpect, "hello");
    EXPECT_EQ(x.error(), "hello");
}

TEST(Expected, Deref_OperatorStar) {
    constexpr auto x = nova::expected<int, std::string_view>(9);
    EXPECT_EQ(*x, 9);

    auto y = nova::expected<int, std::string_view>(9);
    *y = 10;
    EXPECT_EQ(*y, 10);
}

TEST(Expected, Deref_OperatorStar_RValue) {
    EXPECT_EQ(( nova::expected<moo, std::string_view>(moo{ 9 }).operator*() ), moo{ 9 });
    auto x = nova::expected<moo, std::string_view>(moo{ 9 });

    constexpr auto create = [](auto&& ex) -> const auto&& {
        return std::move(ex);
    };

    EXPECT_EQ(create(x).operator*(), 9);
}

TEST(Expected, Deref_OperatorArrow) {
    struct S {
        int x;
        const auto& get() const { return x; }
        void set(int p) { x = p; }
    };

    constexpr auto x = nova::expected<S, std::string_view>(S{ 9 });
    EXPECT_EQ(x->get(), 9);

    auto y = nova::expected<S, std::string_view>(S{ 9 });
    y->set(10);
    EXPECT_EQ(y->get(), 10);
}

TEST(Expected, HasValue) {
    constexpr auto x = nova::expected<int, std::string_view>(1);
    EXPECT_TRUE(x.has_value());

    constexpr auto y = nova::expected<int, std::string_view>(nova::unexpect, "a");
    EXPECT_TRUE(not y.has_value());

    EXPECT_TRUE(( nova::expected<int, std::string_view>(1).has_value() ));
}

TEST(Expected, BoolConversion) {
    constexpr auto x = nova::expected<int, std::string_view>(1);
    EXPECT_TRUE(x);

    constexpr auto y = nova::expected<int, std::string_view>(nova::unexpect, "a");
    EXPECT_TRUE(not y);
}

TEST(Expected, Copy) {
    constexpr auto x = nova::expected<int, std::string_view>(1);
    constexpr auto y = x;
    constexpr auto z{ x };
    EXPECT_EQ(*y, 1);
    EXPECT_EQ(*z, 1);
}

TEST(Expected, Copy_NonTrivial) {
    struct S {
        int x;
    };

    constexpr auto x = nova::expected<S, int>(S{ 9 });
    constexpr auto y = x;
    EXPECT_EQ(y->x, 9);
}

TEST(Expected, Move) {
    constexpr auto x = nova::expected<int, std::string_view>(1);
    constexpr auto y = std::move(x);
    EXPECT_EQ(*y, 1);
}

TEST(Expected, Move_NonTrivial) {
    auto x = nova::expected<std::unique_ptr<int>, int>(std::make_unique<int>(9));
    auto y = std::move(x);
    EXPECT_EQ(**y, 9);

    auto z{ std::move(y) };
    EXPECT_EQ(**z, 9);
}

TEST(Expected, Equality) {
    constexpr auto value_x = nova::expected<int, std::string_view>(1);
    constexpr auto value_y = nova::expected<int, std::string_view>(1);
    constexpr auto value_z = nova::expected<int, std::string_view>(3);
    constexpr auto error_x = nova::expected<int, std::string_view>(nova::unexpect, "a");
    constexpr auto error_y = nova::expected<int, std::string_view>(nova::unexpect, "a");
    constexpr auto error_z = nova::expected<int, std::string_view>(nova::unexpect, "b");

    EXPECT_EQ(value_x, value_y);
    EXPECT_EQ(error_x, error_y);
    EXPECT_NE(value_x, error_y);
    EXPECT_NE(error_x, value_y);

    EXPECT_NE(value_x, value_z);
    EXPECT_NE(error_x, error_z);
}

TEST(Expected_MondadicOps, ValueOr) {
    constexpr auto x = nova::expected<int, std::string_view>(9);
    EXPECT_EQ(x.value_or(2), 9);

    constexpr auto y = nova::expected<int, std::string_view>(nova::unexpect, "a");
    EXPECT_EQ(y.value_or(2), 2);
}

TEST(Expected_MondadicOps, ValueOr_OnRValue) {
    EXPECT_EQ(( nova::expected<moo, std::string_view>(moo{ 1 }).value_or(moo{ 2 }) ), moo{ 1 });
    EXPECT_EQ(( nova::expected<moo, std::string_view>(nova::unexpect, "a").value_or(moo{ 3 }) ), moo{ 3 });
}

TEST(Expected_MondadicOps, ErrorOr) {
    constexpr auto x = nova::expected<int, std::string_view>(9);
    EXPECT_EQ(x.error_or("e"), "e");

    constexpr auto y = nova::expected<int, std::string_view>(nova::unexpect, "a");
    EXPECT_EQ(y.error_or("e"), "a");
}

TEST(Expected_MondadicOps, ErrorOr_OnRValue) {
    EXPECT_EQ(( nova::expected<moo, std::string_view>(moo{ 1 }).error_or("e") ), "e");
    EXPECT_EQ(( nova::expected<moo, std::string_view>(nova::unexpect, "a").error_or("e") ), "a");
}

TEST(Expected_MondadicOps, AndThen) {
    using E = nova::expected<int, std::string_view>;
    constexpr auto x = E(9);
    constexpr auto x2 = [](int y) { return E{ y * 2 }; };
    EXPECT_EQ(x.and_then(x2).value(), 18);
}

TEST(Expected_MondadicOps, AndThen_OnRValue) {
    using E = nova::expected<moo, std::string_view>;
    constexpr auto x2m = [](const moo& x) { return E{ x.x * 2 }; };
    EXPECT_EQ(E(9).and_then(x2m).value().x, 18);
}

TEST(Expected_MondadicOps, AndThen_TypeTransform) {
    using namespace std::literals::string_literals;
    using E = nova::expected<int, std::string_view>;
    using E2 = nova::expected<std::string, std::string_view>;
    constexpr auto x = E(9);
    constexpr auto t = [](int y) { return E2{ std::to_string(y) }; };
    EXPECT_EQ(x.and_then(t).value(), "9"s);
}

TEST(Expected_MondadicOps, OrElse) {
    using E = nova::expected<int, std::string_view>;
    using E2 = nova::expected<int, std::size_t>;
    constexpr auto x = E(nova::unexpect, "error");
    constexpr auto t = [](const std::string_view& y) { return E2{ nova::unexpect, y.size() }; };
    EXPECT_EQ(x.or_else(t).error(), 5);
}
