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

function(isDirty)
    git(status --porcelain)
    if(NOT "${stdout}" STREQUAL "")
        set(ENV{GIT_DIRTY} "true")
    else()
        set(ENV{GIT_DIRTY} "false")
    endif()
endfunction()

githash()
isDirty()
message("Git hash: $ENV{GIT_HASH}")
message("Git dirty: $ENV{GIT_DIRTY}")

configure_file("nova.hh.in" "${CMAKE_CURRENT_LIST_DIR}/../include/nova/nova.hh")
