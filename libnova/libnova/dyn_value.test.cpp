#include <libnova/dyn_value.hpp>

#include <libnova/test_utils.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace {

using string_map = std::map<std::string, std::string>;
using int_map = std::map<std::string, int>;

auto sample() -> nova::dyn_value {
    return nova::dyn_value::map({
        { "debugger", "gdb" },
        { "debugger-args", nova::dyn_value::seq({ "--args", "-q" }) },
        { "jobs", 8 },
        { "ratio", 0.5 },
        { "mount", true },
        { "nothing", nullptr },
        { "cmake", nova::dyn_value::map({
            { "definitions", nova::dyn_value::map({
                { "CMAKE_CXX_STANDARD", 23 },
                { "CMAKE_BUILD_TYPE", "Debug" },
            }) },
        }) },
        { "files", nova::dyn_value::map({ { "foo.cpp", "c++" } }) },
    });
}

} // namespace

// --- Paths ---

TEST(DynPath, Dotted) {
    EXPECT_EQ(nova::dyn_path("a.b.c").segments(), (std::vector<std::string>{ "a", "b", "c" }));
    EXPECT_EQ(nova::dyn_path("files.foo\\.cpp").segments(), (std::vector<std::string>{ "files", "foo.cpp" }));
    EXPECT_TRUE(nova::dyn_path("").empty());
}

TEST(DynPath, SegmentsAreVerbatim) {
    EXPECT_EQ((nova::dyn_path{ "files", "foo.cpp" }).segments(), (std::vector<std::string>{ "files", "foo.cpp" }));
    EXPECT_EQ((nova::dyn_path{ "a.b" }).segments(), (std::vector<std::string>{ "a.b" }));
}

TEST(DynPath, Append) {
    const auto base = nova::dyn_path("cmake");
    EXPECT_EQ(base / "definitions" / "X", nova::dyn_path("cmake.definitions.X"));
    EXPECT_EQ(nova::dyn_path("list") / 2, nova::dyn_path("list.2"));
}

TEST(DynPath, ToStringEscapes) {
    EXPECT_EQ((nova::dyn_path{ "files", "foo.cpp" }).to_string(), "files.foo\\.cpp");
    EXPECT_EQ(nova::dyn_path().to_string(), "<root>");
}

// --- Lookup ---

TEST(DynValue, Lookup) {
    const auto doc = sample();

    EXPECT_EQ(doc.lookup<std::string>("debugger"), "gdb");
    EXPECT_EQ(doc.lookup<int>("jobs"), 8);
    EXPECT_EQ(doc.lookup<bool>("mount"), true);
    EXPECT_EQ(doc.lookup<int>("cmake.definitions.CMAKE_CXX_STANDARD"), 23);
    EXPECT_EQ(doc.lookup<std::string>("debugger-args.1"), "-q");
    EXPECT_EQ(doc.lookup<std::string>({ "files", "foo.cpp" }), "c++");
    EXPECT_EQ(nova::dyn_value(42).lookup<int>(), 42);
}

TEST(DynValue, Lookup_Numbers) {
    const auto doc = sample();

    EXPECT_DOUBLE_EQ(doc.lookup<double>("jobs"), 8.0);
    EXPECT_FLOAT_EQ(doc.lookup<float>("ratio"), 0.5F);
    EXPECT_THROWN_MESSAGE(std::ignore = doc.lookup<int>("ratio"), "`ratio`: expected int32, got float");
    EXPECT_THROWN_MESSAGE(std::ignore = nova::dyn_value(300).lookup<std::uint8_t>(), "expected uint8, got 300 \\(out of range\\)");
}

TEST(DynValue, Lookup_MissingThrows) {
    EXPECT_THROWN_MESSAGE(std::ignore = sample().lookup<int>("cmake.missing"), "Missing `cmake.missing`");
}

TEST(DynValue, Lookup_NullIsMissing) {
    const auto doc = sample();
    EXPECT_THROWN_MESSAGE(std::ignore = doc.lookup<int>("nothing"), "Missing `nothing`");
    EXPECT_EQ(doc.lookup<int>("nothing", 7), 7);
    EXPECT_EQ(doc.lookup<int>("nothing.deeper", 7), 7);
}

TEST(DynValue, Lookup_Fallback) {
    const auto doc = sample();
    EXPECT_EQ(doc.lookup<int>("missing", 7), 7);
    EXPECT_EQ(doc.lookup("jobs", 1), 8);
    EXPECT_EQ(doc.lookup<std::string>("missing", "lldb"), "lldb");
}

TEST(DynValue, Lookup_WrongTypeThrowsEvenWithFallback) {
    EXPECT_THROWN_MESSAGE(std::ignore = sample().lookup<bool>("debugger", false), "`debugger`: expected bool, got string");
}

TEST(DynValue, Lookup_NoStringConversion) {
    const auto doc = sample();
    const auto path = nova::dyn_path("cmake.definitions.CMAKE_CXX_STANDARD");

    EXPECT_THROWN_MESSAGE(std::ignore = doc.lookup<std::string>(path), "expected string, got int");
    EXPECT_EQ(fmt::format("{}", doc.at(path)), "23");
    EXPECT_EQ(fmt::format("{}", nova::dyn_value(1.10)), "1.1");
    EXPECT_EQ(fmt::format("{}", nova::dyn_value(true)), "true");
    EXPECT_THROWN_MESSAGE(std::ignore = fmt::format("{}", doc.at("cmake")), "Cannot format a map as text");
}

TEST(DynValue, Lookup_WalkingThroughScalarThrows) {
    const auto doc = sample();
    EXPECT_THROWN_MESSAGE(std::ignore = doc.lookup<int>("jobs.deeper", 7), "`jobs`: expected map or seq, got int");
    EXPECT_THROWN_MESSAGE(std::ignore = doc.lookup<int>("debugger-args.first"), "`debugger-args`: expected an index, got key `first`");
}

TEST(DynValue, Lookup_Containers) {
    const auto doc = sample();

    EXPECT_EQ(
        doc.lookup<std::vector<std::string>>("debugger-args"),
        (std::vector<std::string>{ "--args", "-q" })
    );

    EXPECT_EQ(
        (doc.lookup<std::map<std::string, nova::dyn_value>>("cmake.definitions")).at("CMAKE_BUILD_TYPE"),
        nova::dyn_value("Debug")
    );

    EXPECT_THROWN_MESSAGE(
        std::ignore = doc.lookup<string_map>("cmake.definitions"),
        "`cmake.definitions.CMAKE_CXX_STANDARD`: expected string, got int"
    );

    EXPECT_THROWN_MESSAGE(
        std::ignore = nova::dyn_value::seq({ 1, "two" }).lookup<std::vector<int>>(),
        "`1`: expected int32, got string"
    );
}

TEST(DynValue, Lookup_ContainersSkipNullInMapsOnly) {
    const auto map = nova::dyn_value::map({ { "a", 1 }, { "b", nullptr } });
    EXPECT_EQ(map.lookup<int_map>(), (int_map{ { "a", 1 } }));

    const auto seq = nova::dyn_value::seq({ 1, nullptr });
    EXPECT_THROWN_MESSAGE(std::ignore = seq.lookup<std::vector<int>>(), "`1`: expected int32, got null");
}

TEST(DynValue, Find) {
    const auto doc = sample();
    EXPECT_EQ(doc.find<std::string>("debugger"), "gdb");
    EXPECT_EQ(doc.find<std::string>("docker.baldr-path"), std::nullopt);
    EXPECT_EQ(doc.find<std::string>("nothing"), std::nullopt);
    EXPECT_THROWN_MESSAGE(std::ignore = doc.find<int>("debugger"), "expected int32, got string");
}

TEST(DynValue, Is_AgreesWithLookup) {
    const auto doc = sample();
    const auto paths = std::vector<nova::dyn_path>{
        "debugger", "jobs", "ratio", "mount", "nothing", "missing", "jobs.deeper",
        "debugger-args", "debugger-args.0", "debugger-args.9", "debugger-args.first",
        "cmake.definitions", { "files", "foo.cpp" }, { },
    };

    const auto agrees = [&doc]<typename T>(const nova::dyn_path& path) {
        auto succeeds = true;
        try {
            std::ignore = doc.lookup<T>(path);
        } catch (const nova::exception&) {
            succeeds = false;
        }
        EXPECT_EQ(doc.is<T>(path), succeeds) << "path: " << path.to_string();
    };

    for (const auto& path : paths) {
        agrees.operator()<bool>(path);
        agrees.operator()<int>(path);
        agrees.operator()<double>(path);
        agrees.operator()<std::string>(path);
        agrees.operator()<std::vector<std::string>>(path);
        agrees.operator()<string_map>(path);
        agrees.operator()<nova::dyn_value::map_t>(path);
        agrees.operator()<nova::dyn_value>(path);
    }
}

TEST(DynValue, At) {
    const auto doc = sample();
    EXPECT_EQ(doc.at("cmake.definitions").as_map().size(), 2);
    EXPECT_THROWN_MESSAGE(std::ignore = doc.at("nothing"), "Missing `nothing`");
    EXPECT_THROWN_MESSAGE(std::ignore = doc.at("jobs").as_seq(), "`<root>`: expected seq, got int");
}

// --- Extension ---

namespace {

struct version {
    int major;
    int minor;

    auto operator==(const version&) const -> bool = default;
};

} // namespace

template <>
struct nova::dyn_convert<version> {
    [[nodiscard]] static auto name() -> std::string {
        return "version";
    }

    [[nodiscard]] static auto convert(const dyn_value& node) -> expected<version, dyn_mismatch> {
        const auto* text = std::get_if<std::string>(&node.data());

        auto ret = version{ };
        if (text == nullptr or std::sscanf(text->c_str(), "%d.%d", &ret.major, &ret.minor) != 2) {     // NOLINT
            return unexpected<dyn_mismatch>{ {
                .where = { },
                .expected = name(),
                .got = std::string(node.type_name()) }
            };
        }

        return ret;
    }
};

TEST(DynValue, Extension) {
    const auto doc = nova::dyn_value::map({ { "min", "1.10" }, { "bad", 3 } });
    EXPECT_EQ(doc.lookup<version>("min"), (version{ 1, 10 }));
    EXPECT_TRUE(doc.is<std::vector<version>>() == false);
    EXPECT_THROWN_MESSAGE(std::ignore = doc.lookup<version>("bad"), "`bad`: expected version, got int");
}

// --- Merge ---

TEST(DynValue, Merge) {
    const auto base = nova::dyn_value::map({
        { "build-type", "Release" },
        { "args", nova::dyn_value::seq({ "--a", "--b" }) },
        { "env", nova::dyn_value::map({ { "CC", "gcc" }, { "CXX", "g++" } }) },
    });

    const auto overlay = nova::dyn_value::map({
        { "build-type", nullptr },
        { "args", nova::dyn_value::seq({ "--c" }) },
        { "env", nova::dyn_value::map({ { "CC", "clang" }, { "CXX", nullptr } }) },
        { "new", nova::dyn_value::map({ { "kept", 1 }, { "dropped", nullptr } }) },
    });

    const auto merged = nova::dyn_value::merge(base, overlay);

    EXPECT_EQ(merged, nova::dyn_value::map({
        { "args", nova::dyn_value::seq({ "--c" }) },
        { "env", nova::dyn_value::map({ { "CC", "clang" } }) },
        { "new", nova::dyn_value::map({ { "kept", 1 } }) },
    }));
}

TEST(DynValue, Merge_SingleDocumentReadsTheSame) {
    const auto doc = sample();
    const auto merged = nova::dyn_value::merge(nova::dyn_value::map({ }), doc);

    EXPECT_FALSE(merged.as_map().contains("nothing"));
    EXPECT_EQ(doc.find<int>("nothing"), merged.find<int>("nothing"));
    EXPECT_EQ(doc.lookup<int>("jobs"), merged.lookup<int>("jobs"));
}

// --- Reading ---

TEST(DynValue, Parse_EveryFormatGivesTheSameTree) {
    const auto expected = nova::dyn_value::map({
        { "name", "x" },
        { "jobs", 8 },
        { "env", nova::dyn_value::map({ { "CC", "clang" } }) },
    });

    using enum nova::dyn_value::format_e;
    EXPECT_EQ(nova::dyn_value::parse("name: x\njobs: 8\nenv:\n  CC: clang\n", yaml).value(), expected);
    EXPECT_EQ(nova::dyn_value::parse(R"({"name": "x", "jobs": 8, "env": {"CC": "clang"}})", json).value(), expected);
    EXPECT_EQ(nova::dyn_value::parse("name = 'x'\njobs = 8\n[env]\nCC = 'clang'\n", toml).value(), expected);
}

TEST(DynValue, Parse_BlankIsNull) {
    using enum nova::dyn_value::format_e;
    for (const auto format : { yaml, json, toml }) {
        EXPECT_TRUE(nova::dyn_value::parse(" \n", format).value().is_null());
    }
    EXPECT_TRUE(nova::dyn_value::parse("# only a comment\n", yaml).value().is_null());
}

TEST(DynValue, Parse_SyntaxError) {
    const auto doc = nova::dyn_value::parse("{ broken", nova::dyn_value::format_e::json);
    ASSERT_FALSE(doc);
    EXPECT_EQ(doc.error().code, nova::make_error_code(nova::dyn_value::error_e::syntax));
}

TEST(DynValue, FormatOf) {
    using enum nova::dyn_value::format_e;
    EXPECT_EQ(nova::dyn_value::format_of("a/b.yaml").value(), yaml);
    EXPECT_EQ(nova::dyn_value::format_of("b.yml").value(), yaml);
    EXPECT_EQ(nova::dyn_value::format_of("b.json").value(), json);
    EXPECT_EQ(nova::dyn_value::format_of("b.toml").value(), toml);

    const auto unknown = nova::dyn_value::format_of("doc.ini");
    ASSERT_FALSE(unknown);
    EXPECT_EQ(unknown.error().code, nova::make_error_code(nova::dyn_value::error_e::unknown_format));
}

TEST(DynValue, AsConfig_NullIsEmptyMap) {
    EXPECT_EQ(nova::dyn_value::as_config({ }).value(), nova::dyn_value::map({ }));
}

TEST(DynValue, AsConfig_MapIsKept) {
    const auto doc = nova::dyn_value::map({ { "debugger", "lldb" } });
    EXPECT_EQ(nova::dyn_value::as_config(doc).value(), doc);
}

TEST(DynValue, AsConfig_NotAMap) {
    for (const auto& doc : { nova::dyn_value::seq({ 1 }), nova::dyn_value(42), nova::dyn_value("text") }) {
        const auto config = nova::dyn_value::as_config(doc);
        ASSERT_FALSE(config);
        EXPECT_EQ(config.error().code, nova::make_error_code(nova::dyn_value::error_e::not_a_map));
        EXPECT_EQ(config.error().detail, doc.type_name());
    }
}
