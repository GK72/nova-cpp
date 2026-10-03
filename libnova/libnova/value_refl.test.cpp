#include "libnova/value.hpp"
#include "libnova/value_json_refl.hpp"
#include <cstdint>
#include <libnova/value_refl.hpp>

#ifdef NOVA_EXPERIMENTAL_FEATURE_SET

#include <libnova/test_utils.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <optional>
#include <string>
#include <vector>

TEST(AttrRefl, AsErrorFallsBackToReflectionName) {
    struct custom_type {
        int x;
    };

    EXPECT_THROWN_MESSAGE(std::ignore = nova::value(42).as<custom_type>(), "Cannot convert int64 to custom_type");
    EXPECT_THROWN_MESSAGE(std::ignore = nova::value("x").as<int>(), "Cannot convert string to int");
}

struct server_config {
    std::string host;
    int port;
    std::optional<std::string> motd;
};

TEST(AttrRefl, BindAggregate) {
    auto v = nova::value::map({
        { "host", "localhost" },
        { "port", 8080 },
    });

    auto cfg = nova::bind<server_config>(v);
    EXPECT_EQ(cfg.host, "localhost");
    EXPECT_EQ(cfg.port, 8080);
    EXPECT_EQ(cfg.motd, std::nullopt);
}

TEST(AttrRefl, BindAggregateMissingRequired) {
    auto v = nova::value::map({
        { "host", "localhost" },
    });

    EXPECT_THROWN_MESSAGE(nova::bind<server_config>(v), "Missing required key 'port'");
}

struct renamed_config {
    [[=nova::ann::rename{ "server-host" }]] std::string host;
    int port = 1234;
};

TEST(AttrRefl, BindRenamedAndOptional) {
    auto v = nova::value::map({
        { "server-host", "example.com" },
    });

    auto cfg = nova::bind<renamed_config>(v);
    EXPECT_EQ(cfg.host, "example.com");
    EXPECT_EQ(cfg.port, 1234);
}

enum class log_level {
    debug,
    info,
    error
};

struct logging_config {
    log_level level;
    std::vector<int> ports;
};

TEST(AttrRefl, BindEnumAndSeq) {
    auto v = nova::value::map({
        { "level", "info" },
        { "ports", nova::value::seq({ 1, 2 }) },
    });

    auto cfg = nova::bind<logging_config>(v);
    EXPECT_EQ(cfg.level, log_level::info);
    EXPECT_THAT(cfg.ports, testing::ElementsAre(1, 2));
}

TEST(AttrRefl, BindInvalidEnum) {
    auto v = nova::value::map({
        { "level", "bogus" },
        { "ports", nova::value(nova::value::seq_t{}) },
    });

    EXPECT_THROWN_MESSAGE(nova::bind<logging_config>(v), "Invalid enumeration: bogus");
}

struct nested_config {
    server_config server;
};

TEST(AttrRefl, BindNested) {
    auto v = nova::value::map({
        { "server", nova::value::map({
            { "host", "localhost" },
            { "port", 80 },
        }) },
    });

    auto cfg = nova::bind<nested_config>(v);
    EXPECT_EQ(cfg.server.host, "localhost");
    EXPECT_EQ(cfg.server.port, 80);
}

TEST(AttrRefl, BindSequenceOfScalars) {
    auto xs = nova::value::seq({ 1, 2, 3 });
    EXPECT_THAT(nova::bind<std::vector<int>>(xs), testing::ElementsAre(1, 2, 3));
}

TEST(AttrRefl, UnbindAggregate) {
    auto cfg = server_config{ .host = "localhost", .port = 8080, .motd = std::nullopt };
    auto v = nova::unbind(cfg);

    EXPECT_EQ(v["host"].as<std::string>(), "localhost");
    EXPECT_EQ(v["port"].as<int>(), 8080);
    EXPECT_EQ(v.find("motd"), std::nullopt);
}

TEST(AttrRefl, UnbindOptionalPresent) {
    auto cfg = server_config{ .host = "localhost", .port = 8080, .motd = "hi" };
    auto v = nova::unbind(cfg);

    EXPECT_EQ(v["motd"].as<std::string>(), "hi");
}

TEST(AttrRefl, UnbindRenamed) {
    auto cfg = renamed_config{ .host = "example.com", .port = 1234 };
    auto v = nova::unbind(cfg);

    EXPECT_EQ(v["server-host"].as<std::string>(), "example.com");
    EXPECT_EQ(v["port"].as<int>(), 1234);
}

TEST(AttrRefl, UnbindEnumAndSeq) {
    auto cfg = logging_config{ .level = log_level::info, .ports = { 1, 2 } };
    auto v = nova::unbind(cfg);

    EXPECT_EQ(v["level"].as<std::string>(), "info");
    EXPECT_EQ(v["ports"][0].as<int>(), 1);
    EXPECT_EQ(v["ports"][1].as<int>(), 2);
}

TEST(AttrRefl, UnbindNested) {
    auto cfg = nested_config{ .server = { .host = "localhost", .port = 80, .motd = std::nullopt } };
    auto v = nova::unbind(cfg);

    EXPECT_EQ(v["server"]["host"].as<std::string>(), "localhost");
    EXPECT_EQ(v["server"]["port"].as<int>(), 80);
}

TEST(AttrRefl, BindUnbindRoundtrip) {
    auto cfg = logging_config{ .level = log_level::error, .ports = { 80, 443, 8080 } };
    auto roundtripped = nova::bind<logging_config>(nova::unbind(cfg));

    EXPECT_EQ(roundtripped.level, cfg.level);
    EXPECT_EQ(roundtripped.ports, cfg.ports);
}

TEST(AttrRefl, E2E) {
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

    auto input = nova::value::map({
        { "name", "Alice" },
        { "id", 72 },
        { "color", "blue" },
        { "factor", 0.5 },
        { "labels", nova::value::map({ { "key", "value" } }) },
        { "coords", nova::value::seq({ 3, 5 }) }
    });

    auto cfg = nova::bind<config>(input);

    EXPECT_EQ(cfg.name, "Alice");
    EXPECT_EQ(cfg.id, 72);
    EXPECT_EQ(cfg.color, color_e::blue);
    EXPECT_DOUBLE_EQ(cfg.factor, 0.5);
    EXPECT_EQ(cfg.labels.at("key"), "value");
    ASSERT_EQ(cfg.coords.size(), 2);
    EXPECT_EQ(cfg.coords[0], 3);
    EXPECT_EQ(cfg.coords[1], 5);

    constexpr auto json_string =R"json({
    "active": true,
    "color": "blue",
    "coords": [
        3,
        5
    ],
    "factor": 0.5,
    "id": 72,
    "labels": {
        "key": "value"
    },
    "name": "Alice"
})json";

    EXPECT_EQ(nova::unbind_json(cfg, 4), json_string);
}

#endif // NOVA_EXPERIMENTAL_FEATURE_SET
