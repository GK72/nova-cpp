# All test targets are discovered automatically.

# The convention is to use underscores for multi-word filenames.
# The will be replaced with hyphens for target names and they are prefixed with
# `test-`.

# All targets are linked with `nova` target with all dependencies.

find_package(GTest REQUIRED)

include(CTest)
include(GoogleTest)

function(add_test_target TARGET_NAME SOURCE_FILE)
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

function(discover_tests_in_directory)
    file(GLOB TEST_SOURCES "*.cc")
    foreach(TEST_SOURCE ${TEST_SOURCES})
        get_filename_component(MODULE_NAME ${TEST_SOURCE} NAME_WE)
        string(REPLACE "_" "-" MODULE_NAME ${MODULE_NAME})
        add_test_target(test-${MODULE_NAME} ${TEST_SOURCE})
    endforeach()
endfunction()

discover_tests_in_directory()
