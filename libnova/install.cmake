include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS nova
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
    "${CMAKE_CURRENT_LIST_DIR}/Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/novaConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/nova"
)

set_property(TARGET nova PROPERTY VERSION ${NOVA_VERSION})
set_property(TARGET nova PROPERTY SOVERSION ${NOVA_VERSION_MAJOR})
set_property(TARGET nova PROPERTY INTERFACE_Nova_MAJOR_VERSION ${NOVA_VERSION_MAJOR})
set_property(TARGET nova APPEND PROPERTY COMPATIBLE_INTERFACE_STRING Nova_MAJOR_VERSION)

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
