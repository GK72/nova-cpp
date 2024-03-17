add_executable(test-nova
    color.cc
    data.cc
    flat_map.cc
    io.cc
    json.cc
    random.cc
    static_string.cc
    type_traits.cc
    utils.cc
    units.cc
    vec.cc
)

target_link_libraries(test-nova PRIVATE
    nova
    nova-deps

    GTest::gtest_main
    GTest::gtest
)

code_analysis(test-nova PRIVATE)

include(CTest)
include(GoogleTest)

gtest_discover_tests(
    test-nova
    PROPERTIES ENVIRONMENT NOVA_TEST_ENV=1
    DISCOVERY_MODE PRE_TEST
)
