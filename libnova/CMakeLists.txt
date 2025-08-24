cmake_minimum_required(VERSION 3.24)

if(CMAKE_BINARY_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "Building in-source is not supported! Create a build dir and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif()

option(NOVA_EXPERIMENTAL_FEATURE_SET "Enable experimental features that requires compiler/lib support" OFF)

include(version.cmake)
set(NOVA_VERSION_FILE "${CMAKE_CURRENT_LIST_DIR}/libnova/details/version.hpp")
nova_extract_version("${NOVA_VERSION_FILE}")

project("nova" VERSION ${NOVA_VERSION} LANGUAGES CXX)
message(STATUS "Nova version ${NOVA_VERSION}")

include(cmake/novaCompilerWarnings.cmake)
include(cmake/novaCompilerSettings.cmake)
include(dependencies.cmake)

include(CTest)

set(LIBNOVA_PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}")
add_subdirectory(libnova)

include(install.cmake)

set(CPACK_RPM_PACKAGE_SUMMARY "Nova C++ Library")
set(CPACK_RPM_PACKAGE_LICENSE "BSL")
set(CPACK_RPM_PACKAGE_VENDOR "GK72")
# set(CPACK_RPM_DEBUGINFO_PACKAGE ON)
include(CPack)
