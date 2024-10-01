add_library(nova INTERFACE)

file(GLOB_RECURSE NOVA_HEADER_FILES "*.h")

target_sources(nova
    # TODO: When there will be compiled artifacts
    # PUBLIC

    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}
    FILES
    ${NOVA_HEADER_FILES}
)

target_link_libraries(nova INTERFACE
    project_warnings
)

target_include_directories(nova INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<INSTALL_INTERFACE:include>
)

code_analysis(nova INTERFACE)

add_library(nova-deps INTERFACE)

target_link_libraries(nova-deps INTERFACE
    fmt::fmt-header-only
    nlohmann_json::nlohmann_json
    spdlog::spdlog_header_only
    yaml-cpp::yaml-cpp
)
