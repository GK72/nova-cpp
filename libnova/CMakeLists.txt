add_library(libnova INTERFACE)
target_link_libraries(libnova INTERFACE project_warnings)

target_include_directories(libnova INTERFACE
    ${CMAKE_HOME_DIRECTORY}
    # $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    # $<INSTALL_INTERFACE:include>
)

code_analysis(libnova INTERFACE)

add_library(libnova-deps INTERFACE)
target_compile_definitions(libnova-deps INTERFACE SPDLOG_FMT_EXTERNAL)

target_link_libraries(libnova-deps INTERFACE
    nova
    fmt::fmt-header-only
    nlohmann_json::nlohmann_json
    spdlog::spdlog_header_only
)
