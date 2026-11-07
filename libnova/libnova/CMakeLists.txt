file(GLOB_RECURSE NOVA_HEADER_FILES "*.hpp")

add_library(nova)

target_sources(nova PRIVATE
    detail/json.cpp
    detail/toml.cpp
    detail/yaml.cpp
    dyn_value.cpp
    version.cpp
)

# Conan builds have no repository, the recipe passes the values captured at export.
nova_target_build_info(nova
    NAMESPACE nova::detail
    HASH "${NOVA_GIT_HASH}"
    BRANCH "${NOVA_GIT_BRANCH}"
    DIRTY "${NOVA_GIT_DIRTY}"
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
    find_package(scn REQUIRED)
    include(GoogleTest)

    add_test_target(color)
    add_test_target(data)
    add_test_target(dyn-value)
    target_link_libraries(test-dyn-value PRIVATE scn::scn)
    add_test_target(dyn-value-formats)
    add_test_target(error)
    add_test_target(exception)
    add_test_target(expected)
    add_test_target(flat-map)
    add_test_target(io)
    add_test_target(not-null)
    add_test_target(parse)
    add_test_target(random)
    add_test_target(ring)
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
