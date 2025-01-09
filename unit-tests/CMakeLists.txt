find_package(GTest REQUIRED)

include(CTest)
include(GoogleTest)

function(test_file TARGET_NAME SOURCE_FILE)
    add_executable(${TARGET_NAME} ${SOURCE_FILE})

    target_link_libraries(${TARGET_NAME} PRIVATE
        nova
        nova-deps

        GTest::gtest_main
        GTest::gtest
        GTest::gmock
    )

    code_analysis(${TARGET_NAME} PRIVATE)

    gtest_discover_tests(
        ${TARGET_NAME}
        PROPERTIES ENVIRONMENT NOVA_TEST_ENV=1
        DISCOVERY_MODE PRE_TEST
    )
endfunction()

test_file(test-color color.cc)
test_file(test-data data.cc)
test_file(test-error error.cc)
test_file(test-expected expected.cc)
test_file(test-flat-map flat_map.cc)
test_file(test-io io.cc)
test_file(test-json json.cc)
test_file(test-random random.cc)
test_file(test-static-string static_string.cc)
test_file(test-std-extensions std_extensions.cc)
test_file(test-type-traits type_traits.cc)
test_file(test-units units.cc)
test_file(test-utils utils.cc)
test_file(test-vec vec.cc)
test_file(test-yaml yaml.cc)
