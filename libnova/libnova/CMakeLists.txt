file(GLOB_RECURSE NOVA_HEADER_FILES "*.hpp")

add_library(nova
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
    nlohmann_json::nlohmann_json
    spdlog::spdlog_header_only
    yaml-cpp::yaml-cpp
)

code_analysis(nova PRIVATE)

if(NOVA_EXPERIMENTAL_FEATURE_SET)
    target_link_libraries(nova PUBLIC stdc++exp)
endif()

include("${LIBNOVA_PROJECT_ROOT}/install.cmake")
include("${LIBNOVA_PROJECT_ROOT}/test.cmake")
