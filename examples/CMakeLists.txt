add_executable(example-threading
    threading.cc
)

target_link_libraries(example-threading PRIVATE
    nova
)

code_analysis(example-threading PRIVATE)
