cmake_minimum_required(VERSION 3.24)

if(CMAKE_BINARY_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "Building in-source is not supported! Create a build dir and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif()

include(cmake/version.cmake)

nova_extract_version()
project("nova" VERSION ${NOVA_VERSION} LANGUAGES CXX)
message(STATUS "Nova version ${NOVA_VERSION}")

add_library(project_warnings INTERFACE)
include(cmake/compiler-warnings.cmake)
set_project_warnings(project_warnings)

include(cmake/settings.cmake)
include(cmake/dependencies.cmake)

add_subdirectory(include)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(unit-tests)
    add_subdirectory(functional-tests)
endif()

add_subdirectory(examples)

install(TARGETS nova FILE_SET HEADERS)
