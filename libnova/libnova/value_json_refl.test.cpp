#include <libnova/value_json_refl.hpp>

#ifdef NOVA_EXPERIMENTAL_FEATURE_SET

#include <libnova/test_utils.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <map>
#include <optional>
#include <string>
#include <vector>

struct server_config {
    std::string host;
    int port;
    std::optional<std::string> motd;
};

TEST(ValueJsonRefl, BindJson) {
    auto cfg = nova::bind_json<server_config>(R"({"host": "localhost", "port": 8080})");

    EXPECT_EQ(cfg.host, "localhost");
    EXPECT_EQ(cfg.port, 8080);
    EXPECT_EQ(cfg.motd, std::nullopt);
}

TEST(ValueJsonRefl, UnbindJson) {
    auto cfg = server_config{ .host = "localhost", .port = 8080, .motd = "hi" };
    auto text = nova::unbind_json(cfg);

    EXPECT_THAT(text, testing::HasSubstr(R"("host":"localhost")"));
    EXPECT_THAT(text, testing::HasSubstr(R"("port":8080)"));
    EXPECT_THAT(text, testing::HasSubstr(R"("motd":"hi")"));
}

struct logging_config {
    std::vector<int> ports;
    std::map<std::string, std::string> labels;
};

TEST(ValueJsonRefl, BindUnbindJsonRoundtrip) {
    auto cfg = logging_config{ .ports = { 80, 443 }, .labels = { { "env", "prod" } } };
    auto roundtripped = nova::bind_json<logging_config>(nova::unbind_json(cfg));

    EXPECT_EQ(roundtripped.ports, cfg.ports);
    EXPECT_EQ(roundtripped.labels, cfg.labels);
}

#endif // NOVA_EXPERIMENTAL_FEATURE_SET
