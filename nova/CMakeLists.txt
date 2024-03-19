add_executable(pathtracer pathtracer.cpp)
target_link_libraries(pathtracer PRIVATE libnova libnova-deps)
code_analysis(pathtracer PRIVATE)
