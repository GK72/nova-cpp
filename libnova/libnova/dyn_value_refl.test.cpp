#include <libnova/detail/dyn_value_refl.hpp>

#ifdef NOVA_EXPERIMENTAL_FEATURE_SET

#include <libnova/dyn_value.hpp>
#include <libnova/test_utils.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace {

using nova::detail::bind;
using nova::detail::unbind;

struct server_config {
    std::string host;
    int port;
    std::optional<std::string> motd;
};

struct renamed_config {
    [[=nova::detail::ann::rename{ "server-host" }]] std::string host;
    int port = 1234;
};

enum class log_level {
    debug,
    info,
    error
};

struct logging_config {
    log_level level;
    std::vector<int> ports;
};

struct nested_config {
    server_config server;
};

} // namespace

TEST(DynValueRefl, BindAggregate) {
    const auto cfg = bind<server_config>(nova::dyn_value::map({
        { "host", "localhost" },
        { "port", 8080 },
    }));

    EXPECT_EQ(cfg.host, "localhost");
    EXPECT_EQ(cfg.port, 8080);
    EXPECT_EQ(cfg.motd, std::nullopt);
}

TEST(DynValueRefl, BindMissingRequired) {
    EXPECT_THROWN_MESSAGE(
        std::ignore = bind<server_config>(nova::dyn_value::map({ { "host", "localhost" } })),
        "Missing `port`"
    );
}

TEST(DynValueRefl, BindNullIsMissing) {
    const auto cfg = bind<server_config>(nova::dyn_value::map({
        { "host", "localhost" },
        { "port", 80 },
        { "motd", nullptr },
    }));
    EXPECT_EQ(cfg.motd, std::nullopt);

    EXPECT_THROWN_MESSAGE(
        std::ignore = bind<server_config>(nova::dyn_value::map({ { "host", "localhost" }, { "port", nullptr } })),
        "Missing `port`"
    );
}

TEST(DynValueRefl, BindRenamedAndDefault) {
    const auto cfg = bind<renamed_config>(nova::dyn_value::map({ { "server-host", "example.com" } }));
    EXPECT_EQ(cfg.host, "example.com");
    EXPECT_EQ(cfg.port, 1234);
}

TEST(DynValueRefl, BindEnumAndSeq) {
    const auto cfg = bind<logging_config>(nova::dyn_value::map({
        { "level", "info" },
        { "ports", nova::dyn_value::seq({ 1, 2 }) },
    }));

    EXPECT_EQ(cfg.level, log_level::info);
    EXPECT_THAT(cfg.ports, testing::ElementsAre(1, 2));
}

TEST(DynValueRefl, BindInvalidEnum) {
    EXPECT_THROWN_MESSAGE(
        std::ignore = bind<logging_config>(nova::dyn_value::map({ { "level", "bogus" }, { "ports", nova::dyn_value::seq({ }) } })),
        "`level`: Invalid enumeration: bogus"
    );
}

TEST(DynValueRefl, BindErrorsNameThePath) {
    EXPECT_THROWN_MESSAGE(
        std::ignore = bind<nested_config>(nova::dyn_value::map({
            { "server", nova::dyn_value::map({ { "host", "localhost" }, { "port", "eighty" } }) },
        })),
        "`server.port`: expected int32, got string"
    );
    EXPECT_THROWN_MESSAGE(
        std::ignore = bind<logging_config>(nova::dyn_value::map({ { "level", "info" }, { "ports", nova::dyn_value::seq({ 1, 2.5 }) } })),
        "`ports.1`: expected int32, got float"
    );
    EXPECT_THROWN_MESSAGE(
        std::ignore = bind<nested_config>(nova::dyn_value::map({ { "server", 1 } })),
        "`server`: expected map, got int"
    );
}

TEST(DynValueRefl, BindNested) {
    const auto cfg = bind<nested_config>(nova::dyn_value::map({
        { "server", nova::dyn_value::map({ { "host", "localhost" }, { "port", 80 } }) },
    }));

    EXPECT_EQ(cfg.server.host, "localhost");
    EXPECT_EQ(cfg.server.port, 80);
}

TEST(DynValueRefl, BindSequenceOfScalars) {
    EXPECT_THAT(bind<std::vector<int>>(nova::dyn_value::seq({ 1, 2, 3 })), testing::ElementsAre(1, 2, 3));
}

TEST(DynValueRefl, UnbindAggregate) {
    const auto doc = unbind(server_config{ .host = "localhost", .port = 8080, .motd = std::nullopt });
    EXPECT_EQ(doc, nova::dyn_value::map({ { "host", "localhost" }, { "port", 8080 } }));
}

TEST(DynValueRefl, UnbindOptionalPresent) {
    const auto doc = unbind(server_config{ .host = "localhost", .port = 8080, .motd = "hi" });
    EXPECT_EQ(doc.lookup<std::string>("motd"), "hi");
}

TEST(DynValueRefl, UnbindRenamed) {
    const auto doc = unbind(renamed_config{ .host = "example.com", .port = 1234 });
    EXPECT_EQ(doc, nova::dyn_value::map({ { "server-host", "example.com" }, { "port", 1234 } }));
}

TEST(DynValueRefl, UnbindEnumAndSeq) {
    const auto doc = unbind(logging_config{ .level = log_level::info, .ports = { 1, 2 } });
    EXPECT_EQ(doc, nova::dyn_value::map({ { "level", "info" }, { "ports", nova::dyn_value::seq({ 1, 2 }) } }));
}

TEST(DynValueRefl, RoundTrip) {
    const auto cfg = logging_config{ .level = log_level::error, .ports = { 80, 443, 8080 } };
    const auto back = bind<logging_config>(unbind(cfg));

    EXPECT_EQ(back.level, cfg.level);
    EXPECT_EQ(back.ports, cfg.ports);
}

TEST(DynValueRefl, E2E) {
    enum class color_e { red, green, blue };
    struct config {
        std::string name;
        std::uint16_t id;
        bool active = true;
        color_e color;
        float factor;
        std::map<std::string, std::string> labels;
        std::vector<int> coords;
    };

    const auto input = nova::dyn_value::parse(R"yaml(
name: Alice
id: 72
color: blue
factor: 0.1
labels:
  key: value
coords: [3, 5]
)yaml", nova::dyn_value::format_e::yaml).value();

    const auto cfg = bind<config>(input);

    EXPECT_EQ(cfg.name, "Alice");
    EXPECT_EQ(cfg.id, 72);
    EXPECT_TRUE(cfg.active);
    EXPECT_EQ(cfg.color, color_e::blue);
    EXPECT_FLOAT_EQ(cfg.factor, 0.1F);
    EXPECT_EQ(cfg.labels.at("key"), "value");
    EXPECT_THAT(cfg.coords, testing::ElementsAre(3, 5));

    constexpr auto json = R"json({
    "active": true,
    "color": "blue",
    "coords": [
        3,
        5
    ],
    "factor": 0.1,
    "id": 72,
    "labels": {
        "key": "value"
    },
    "name": "Alice"
}
)json";

    EXPECT_EQ(unbind(cfg).dump(nova::dyn_value::format_e::json).value(), json);
}

#endif // NOVA_EXPERIMENTAL_FEATURE_SET
