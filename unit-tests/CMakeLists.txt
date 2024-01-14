add_executable(test-nova
    color.cc
    data.cc
    flat_map.cc
    random.cc
    static_string.cc
    type_traits.cc
    utils.cc
    units.cc
    vec.cc
)

target_link_libraries(test-nova PRIVATE
    nova
    gtest_main
    gtest
)

code_analysis(test-nova PRIVATE)

add_test(test-nova test-nova)
