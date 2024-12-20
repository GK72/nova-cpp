add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../src")

add_library(nova-headers INTERFACE)

file(GLOB_RECURSE NOVA_HEADER_FILES_DEPRECATED "${CMAKE_CURRENT_LIST_DIR}/*.h")
file(GLOB_RECURSE NOVA_HEADER_FILES "${CMAKE_CURRENT_LIST_DIR}/*.hh")

target_sources(nova-headers
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../include"
    FILES
    ${NOVA_HEADER_FILES_DEPRECATED}
    ${NOVA_HEADER_FILES}
)

target_link_libraries(nova-headers INTERFACE
    project_warnings
)

target_include_directories(nova-headers INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<INSTALL_INTERFACE:include>
)

code_analysis(nova-headers INTERFACE)

add_library(nova-deps INTERFACE)

target_link_libraries(nova-deps INTERFACE
    fmt::fmt-header-only
    nlohmann_json::nlohmann_json
    spdlog::spdlog_header_only
    yaml-cpp::yaml-cpp
)

add_library(nova INTERFACE)

target_link_libraries(nova INTERFACE
    nova-headers
    nova-tcp
)

if(NOVA_EXPERIMENTAL_FEATURE_SET)
    target_link_libraries(nova INTERFACE stdc++exp)
endif()
