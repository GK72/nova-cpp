cmake_minimum_required(VERSION 3.12)

if (CMAKE_BINARY_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "Building in-source is not supported! Create a build dir and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif()

project("Nova")

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)

add_library(project_warnings INTERFACE)
include(cmake/compilerWarnings.cmake)
set_project_warnings(project_warnings)

include(cmake/settings.cmake)

add_subdirectory(tests)
