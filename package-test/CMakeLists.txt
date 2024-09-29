cmake_minimum_required(VERSION 3.15)
project(PackageTest CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(nova CONFIG REQUIRED)

add_executable(package-test main.cc)
target_link_libraries(package-test
    nova::nova
)
