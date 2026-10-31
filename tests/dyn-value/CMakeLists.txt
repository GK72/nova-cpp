add_executable(ft-dyn-value dyn_value.cc)
target_link_libraries(ft-dyn-value PRIVATE nova::nova)
code_analysis(ft-dyn-value PRIVATE)

foreach(case formats config-comments config-list missing directory syntax-error value-message)
    add_test(NAME ft-dyn-value-${case} COMMAND ft-dyn-value ${case} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()

foreach(case write-every-format write-replace write-unsupported write-unknown-format)
    add_test(NAME ft-dyn-value-${case} COMMAND ft-dyn-value ${case} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(ft-dyn-value-${case} PROPERTIES ENVIRONMENT "FT_OUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/out-${case}")
endforeach()
