include("${CMAKE_CURRENT_LIST_DIR}/cpm.cmake")

CPMAddPackage("gh:fmtlib/fmt#10.2.1")
CPMAddPackage("gh:gabime/spdlog#v1.13.0")
CPMAddPackage("gh:google/googletest#v1.14.0")
CPMAddPackage("gh:nlohmann/json#v3.11.3")

CPMAddPackage(
    NAME benchmark
    GITHUB_REPOSITORY google/benchmark
    VERSION 1.8.3
    OPTIONS "BENCHMARK_ENABLE_TESTING OFF"
)
