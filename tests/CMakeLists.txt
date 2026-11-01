cmake_minimum_required(VERSION 3.24)
project("nova-tests")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

find_package(nova REQUIRED)

include(CTest)
include(novaCompilerWarnings)
include(novaCompilerSettings)
include(novaBuildInfo)

add_subdirectory(build-info)

# The tests compile against the installed headers, a copy of the sources: their
# coverage is recorded as the sources' (`<install>/include=<source>`).
if(COVERAGE AND COVERAGE_PREFIX_MAP)
    add_compile_options("-fprofile-prefix-map=${COVERAGE_PREFIX_MAP}")
endif()

add_subdirectory(io)
add_subdirectory(log)
