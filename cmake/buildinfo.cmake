include(${CMAKE_CURRENT_LIST_DIR}/version.cmake OPTIONAL)

macro(git)
    execute_process(
        COMMAND git ${ARGV}
        RESULT_VARIABLE exit_code
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endmacro()

function(githash)
    git(show --no-patch "--format=%H")
    set(ENV{GIT_HASH} "${stdout}")
endfunction()

function(gitdirty)
    git(status --porcelain)
    if(NOT "${stdout}" STREQUAL "")
        set(ENV{GIT_HASH} "$ENV{GIT_HASH}-dirty")
    endif()
endfunction()

function(nova_extract_version)
    if(NOT "$ENV{NOVA_VERSION}" STREQUAL "")
        message("Reading version from `version.cmake`")
        string(REGEX MATCH "([0-9]+).([0-9]+).([0-9]+)" _ "$ENV{NOVA_VERSION}")
        if(NOT CMAKE_MATCH_COUNT EQUAL 3)
            message(FATAL_ERROR "Could not extract version")
        endif()
    else()
        message("Reading version from `conanfile.py`")
        file(READ "${CMAKE_CURRENT_LIST_DIR}/../conanfile.py" file_contents)
        string(REGEX MATCH ".*version = \"([0-9]+).([0-9]+).([0-9]+)\"" _ "${file_contents}")
    endif()

    set(ENV{LIB_VERSION_MAJOR} ${CMAKE_MATCH_1})
    set(ENV{LIB_VERSION_MINOR} ${CMAKE_MATCH_2})
    set(ENV{LIB_VERSION_PATCH} ${CMAKE_MATCH_3})
    if("$ENV{NOVA_VERSION}" STREQUAL "")
        set(ENV{NOVA_VERSION} "$ENV{LIB_VERSION_MAJOR}.$ENV{LIB_VERSION_MINOR}.$ENV{LIB_VERSION_PATCH}")
    endif()
endfunction()

function(buildinfo)
    githash()
    gitdirty()
    nova_extract_version()
    message("Git hash: $ENV{GIT_HASH}")

    configure_file("${CMAKE_CURRENT_LIST_DIR}/nova.hh.in" "${CMAKE_CURRENT_LIST_DIR}/../include/nova/nova.hh")
endfunction()

buildinfo()
