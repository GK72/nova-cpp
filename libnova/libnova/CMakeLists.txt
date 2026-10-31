file(GLOB_RECURSE NOVA_HEADER_FILES "*.hpp")

add_library(nova)

target_sources(nova PRIVATE
    detail/json.cpp
    detail/toml.cpp
    detail/yaml.cpp
    dyn_value.cpp
    nova.cpp
)

target_sources(nova PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${LIBNOVA_PROJECT_ROOT}"
    FILES "${NOVA_HEADER_FILES}"
)

target_include_directories(nova PUBLIC
    "$<BUILD_INTERFACE:${LIBNOVA_PROJECT_ROOT}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

target_link_libraries(nova PUBLIC
    fmt::fmt-header-only
    spdlog::spdlog_header_only
)

find_package(nlohmann_json REQUIRED)
find_package(tomlplusplus REQUIRED)

# The formats are behind compiled sources (`detail/*.cpp`): consumers need no
# headers of them. The header-only ones are not needed at all; yaml-cpp is
# needed for linking the static library only.
target_link_libraries(nova PRIVATE
    $<BUILD_INTERFACE:nlohmann_json::nlohmann_json>
    $<BUILD_INTERFACE:tomlplusplus::tomlplusplus>
    yaml-cpp::yaml-cpp
)

code_analysis(nova PRIVATE)

if(NOVA_EXPERIMENTAL_FEATURE_SET)
    target_link_libraries(nova PUBLIC stdc++exp)
endif()

include(test.cmake)

if(BUILD_TESTING)
    find_package(GTest REQUIRED)
    include(GoogleTest)

    add_test_target(color)
    add_test_target(data)
    add_test_target(dyn-value)
    add_test_target(dyn-value-formats)
    add_test_target(error)
    add_test_target(exception)
    add_test_target(expected)
    add_test_target(flat-map)
    add_test_target(io)
    add_test_target(not-null)
    add_test_target(parse)
    add_test_target(random)
    add_test_target(static-string)
    add_test_target(std-extensions)
    add_test_target(type-traits)
    add_test_target(units)
    add_test_target(utils)
    add_test_target(vec)

    if(NOVA_EXPERIMENTAL_FEATURE_SET)
        add_test_target(refl)
        add_test_target(dyn-value-refl)
    endif()

    find_package(benchmark REQUIRED)

    add_bench_target(serializer)
    add_bench_target(log)
endif()

add_executable(example-log log.ex.cpp)
target_link_libraries(example-log PRIVATE nova)
code_analysis(example-log PRIVATE)
