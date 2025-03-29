find_package(benchmark REQUIRED)

add_executable(bench-serializer serializer.cc)
target_link_libraries(bench-serializer PRIVATE nova benchmark::benchmark)
