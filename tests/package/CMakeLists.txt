cmake_minimum_required(VERSION 3.15)
project(PackageTest CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

find_package(nova REQUIRED)

add_executable(package-test main.cpp)
target_link_libraries(package-test nova::nova)
