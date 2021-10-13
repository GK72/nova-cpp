add_executable (sandbox sandbox.cpp)

target_include_directories(sandbox PRIVATE
    ${CMAKE_HOME_DIRECTORY}/src
)

if (UNIX)
    target_link_libraries(sandbox PRIVATE pthread)
endif()
