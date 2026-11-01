cmake_minimum_required(VERSION 3.15)
project(PackageTest VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

find_package(nova REQUIRED)
find_package(GTest REQUIRED)

include(novaCompilerSettings)
include(novaBuildInfo)

add_executable(package-test main.cpp)
target_link_libraries(package-test nova::nova)
nova_target_build_info(package-test NAMESPACE package_test)

add_executable(package-test-dependencies test.cpp)
target_link_libraries(package-test-dependencies PRIVATE GTest::gtest_main)
