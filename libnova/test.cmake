function(add_test_target MODULE_NAME)
    set(target "test-${MODULE_NAME}")
    set(source_file "${MODULE_NAME}.test.cpp")
    string(REPLACE "-" "_" source_file ${source_file})
    add_executable("${target}" ${source_file})

    target_link_libraries(${target} PRIVATE
        nova

        GTest::gtest_main
        GTest::gtest
        GTest::gmock
    )

    code_analysis(${target} PRIVATE)

    gtest_discover_tests(
        ${target}
        PROPERTIES ENVIRONMENT NOVA_TEST_ENV=1
        DISCOVERY_MODE PRE_TEST
    )

    add_test(NAME ${target} COMMAND ${target})
endfunction()

if(BUILD_TESTS)
    find_package(GTest REQUIRED)
    include(GoogleTest)

    add_test_target(cast)
    add_test_target(color)
    add_test_target(data)
    add_test_target(error)
    add_test_target(expected)
    add_test_target(flat-map)
    add_test_target(io)
    add_test_target(json)
    add_test_target(not-null)
    add_test_target(parse)
    add_test_target(random)
    add_test_target(static-string)
    add_test_target(std-extensions)
    add_test_target(type-traits)
    add_test_target(units)
    add_test_target(utils)
    add_test_target(vec)
    add_test_target(yaml)
endif()
