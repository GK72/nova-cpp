cmake_minimum_required(VERSION 3.24)

if(CMAKE_BINARY_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "Building in-source is not supported! Create a build dir and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif()

option(BUILD_TESTS "Build tests" OFF)
option(BUILD_BENCH "Build benchmark tests" OFF)
option(NOVA_EXPERIMENTAL_FEATURE_SET "Enable experimental features that requires compiler/lib support" OFF)

include(cmake/buildinfo.cmake)

project("nova" VERSION $ENV{NOVA_VERSION} LANGUAGES CXX)
message(STATUS "Nova version: $ENV{NOVA_VERSION}")

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

if(BUILD_BENCH)
    add_subdirectory(bench)
endif()

add_subdirectory(examples)

install(TARGETS nova FILE_SET HEADERS)
