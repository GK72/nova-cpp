# Warnings of `code_analysis()` follow the visibility: `PRIVATE` is for the
# sources of the target, its consumers get nothing.
add_library(ft-code-analysis OBJECT code_analysis.cc)
code_analysis(ft-code-analysis PRIVATE)

get_target_property(own_options ft-code-analysis COMPILE_OPTIONS)
get_target_property(consumer_options ft-code-analysis INTERFACE_COMPILE_OPTIONS)

if(NOT own_options)
    message(FATAL_ERROR "code_analysis(PRIVATE) did not set the warnings of the target")
endif()
if(consumer_options)
    message(FATAL_ERROR "code_analysis(PRIVATE) leaked to consumers: ${consumer_options}")
endif()
