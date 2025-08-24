add_executable(ft-log log.cc)
target_link_libraries(ft-log PRIVATE nova::nova)
code_analysis(ft-log PRIVATE)

add_test(NAME ft-log COMMAND ft-log WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set_tests_properties(ft-log PROPERTIES ENVIRONMENT "SPDLOG_LEVEL=nova=info,nova2=debug")
