cmake_minimum_required(VERSION 3.24)
project("examples")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

find_package(nova REQUIRED)

include(novaCompilerWarnings)
include(novaCompilerSettings)

add_executable(shielded shielded.cc)
target_link_libraries(shielded PRIVATE nova::nova)
