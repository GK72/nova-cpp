add_executable(ft-build-info build_info.cpp)
target_link_libraries(ft-build-info PRIVATE nova::nova)
nova_target_build_info(ft-build-info NAMESPACE ft VERSION 1.2.3)
code_analysis(ft-build-info PRIVATE)

# Expected values straight from git, independent of `novaBuildInfo`.
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE expected_hash
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE expected_branch
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

string(REGEX REPLACE "[^0-9A-Za-z.-]" "-" expected_branch "${expected_branch}")

add_test(NAME ft-build-info COMMAND ft-build-info "${expected_hash}" "${expected_branch}")

# Overrides, with characters that are not allowed in SemVer or break a C++ literal.
add_executable(ft-build-info-override build_info.cpp)
target_link_libraries(ft-build-info-override PRIVATE nova::nova)
nova_target_build_info(ft-build-info-override
    NAMESPACE ft
    VERSION 1.2.3
    HASH abc1234
    BRANCH "u/chris/a\"b"
    DIRTY clean
)
code_analysis(ft-build-info-override PRIVATE)

add_test(NAME ft-build-info-override COMMAND ft-build-info-override abc1234 u-chris-a-b)

# Same test source for both, the generated header is named after the target.
target_compile_definitions(ft-build-info PRIVATE BUILD_INFO_HEADER=<ft-build-info/gen/build_info.hpp>)
target_compile_definitions(ft-build-info-override PRIVATE BUILD_INFO_HEADER=<ft-build-info-override/gen/build_info.hpp>)
