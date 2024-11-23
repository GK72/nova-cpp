find_program(CCACHE ccache)
if(CCACHE)
    message("Using ccache")
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
else()
    message("Ccache cannot be found")
endif()

set(CMAKE_CXX_EXTENSIONS OFF)

if(NOVA_EXPERIMENTAL_FEATURE_SET)
    set(CMAKE_CXX_STANDARD 26)
    add_compile_definitions(NOVA_EXPERIMENTAL_FEATURE_SET)
else()
    set(CMAKE_CXX_STANDARD 20)
endif()

if(COVERAGE)
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-g -O0 -fno-inline -fprofile-arcs -ftest-coverage")
endif()

if(SANITIZERS)
    if(SANITIZERS STREQUAL asan)
        set(SANITIZER_LIST -fsanitize=address -fsanitize=leak -fsanitize=undefined)
    endif()
    if(SANITIZERS STREQUAL tsan)
        set(SANITIZER_LIST -fsanitize=thread)
    endif()
endif()

function(code_analysis TARGET VISIBILITY)
    target_link_libraries(${TARGET} ${VISIBILITY} project_warnings)

    if(${SANITIZERS} MATCHES "[at]san")
        target_compile_options(${TARGET} ${VISIBILITY} ${SANITIZER_LIST})
        target_link_options(${TARGET} ${VISIBILITY} ${SANITIZER_LIST})
        message("Code analysis is turned on for ${TARGET} with ${SANITIZER_LIST}")
    else()
        message("Code analysis is turned on for ${TARGET}")
    endif()
endfunction()

# TODO(win):
# - D9025: overriding '/MDd' with '/MTd'
# - D9025: overriding '/W3' with '/W4'
if(WIN32)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
