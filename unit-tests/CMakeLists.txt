add_executable(test-nova
    utils.cc
)

target_link_libraries(test-nova PRIVATE
    nova
    gtest_main
    gtest
)

code_analysis(test-nova PRIVATE)

add_test(test-nova test-nova)
