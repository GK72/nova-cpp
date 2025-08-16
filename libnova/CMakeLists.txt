file(GLOB_RECURSE NOVA_IMPLEMENTATION_FILES "libnova/*.cpp")
file(GLOB_RECURSE NOVA_HEADER_FILES "libnova/*.hpp")

add_library(nova "${NOVA_IMPLEMENTATION_FILES}")

target_sources(nova PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_LIST_DIR}"
    FILES "${NOVA_HEADER_FILES}"
)

target_include_directories(nova PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)

if(NOVA_EXPERIMENTAL_FEATURE_SET)
    target_link_libraries(nova PUBLIC stdc++exp)
endif()

# code_analysis(nova PRIVATE)           # TODO: Link to tests

add_library(nova-deps INTERFACE)

target_link_libraries(nova-deps INTERFACE
    fmt::fmt-header-only
    nlohmann_json::nlohmann_json
    spdlog::spdlog_header_only
    yaml-cpp::yaml-cpp
)

install(TARGETS nova nova-deps
    EXPORT NovaTargets
    FILE_SET HEADERS
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(EXPORT NovaTargets
    FILE novaTargets.cmake
    NAMESPACE nova::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/nova"
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/novaConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/nova"
)

set_property(TARGET nova PROPERTY VERSION ${NOVA_VERSION})
set_property(TARGET nova PROPERTY SOVERSION ${NOVA_VERSION_MAJOR})
set_property(TARGET nova PROPERTY INTERFACE_Nova_MAJOR_VERSION ${NOVA_VERSION_MAJOR})
set_property(TARGET nova APPEND PROPERTY COMPATIBLE_INTERFACE_STRING Nova_MAJOR_VERSION)

# generate the version file for the config file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/novaConfigVersion.cmake"
    VERSION "${NOVA_VERSION}"
    COMPATIBILITY AnyNewerVersion
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/novaConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/novaConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/nova"
)
