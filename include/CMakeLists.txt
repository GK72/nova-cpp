add_library(nova INTERFACE)
target_link_libraries(nova INTERFACE project_warnings)

target_include_directories(nova INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    $<INSTALL_INTERFACE:include>
)

code_analysis(nova INTERFACE)
