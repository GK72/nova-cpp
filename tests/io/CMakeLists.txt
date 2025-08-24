add_executable(ft-io io.cc)
target_link_libraries(ft-io PRIVATE nova::nova)
code_analysis(ft-io PRIVATE)

add_test(NAME ft-io-txt COMMAND ft-io io-input.txt WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME ft-io-bin COMMAND ft-io io-input.bin WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME ft-io-invalid-file COMMAND ft-io nothing.txt WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
