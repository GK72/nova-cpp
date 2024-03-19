add_library(libnova-gfx
    pathtracer.cpp
)

target_link_libraries(libnova-gfx PRIVATE project_warnings)

target_include_directories(libnova-gfx PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
    # $<INSTALL_INTERFACE:include>
)

code_analysis(libnova-gfx PUBLIC)

target_link_libraries(libnova-gfx PUBLIC libnova)
