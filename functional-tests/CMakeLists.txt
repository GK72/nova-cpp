include(CTest)

add_executable(ft-io io.cc)
target_link_libraries(ft-io PRIVATE nova nova-deps)
code_analysis(ft-io PRIVATE)
add_test(NAME ft-io-txt COMMAND ft-io io-input.txt WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME ft-io-bin COMMAND ft-io io-input.bin WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_test(NAME ft-io-invalid-file COMMAND ft-io nothing.txt WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(ft-log log.cc)
target_link_libraries(ft-log PRIVATE nova nova-deps)
code_analysis(ft-log PRIVATE)
add_test(NAME ft-log COMMAND ft-log WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set_tests_properties(ft-log PROPERTIES ENVIRONMENT "SPDLOG_LEVEL=nova=info,nova2=debug")
