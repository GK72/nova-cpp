# novaBuildInfo.cmake
#
# Installed with Nova, available to consumers via `include(novaBuildInfo)`.
#
# Provides:
#   `nova_target_build_info(<target> NAMESPACE <ns> [VERSION <x.y.z>]
#                           [WORKING_DIRECTORY <dir>]
#                           [HASH <hash>] [BRANCH <branch>] [DIRTY <dirty>])`
#       Generates `<target>/gen/build_info.hpp`, compiled into the target privately:
#       `<ns>::version()`, `version_major()`, `version_minor()`,
#       `version_patch()`, `git_hash()`, `git_branch()`, `git_dirty()` and
#       `full_version()` (`<version>+<hash>.<branch>[.dirty]`). The strings are
#       literals, nothing allocates. Only the generated source rebuilds when the
#       values change.
#       VERSION defaults to `PROJECT_VERSION`. HASH, BRANCH and DIRTY override
#       the values from the repository, for builds without one (e.g. captured
#       by a Conan recipe on export) or without a branch (a CI checkout of a
#       commit is a detached HEAD, its branch is `HEAD`).
#       Characters of the hash and the branch outside `[0-9A-Za-z.-]` are
#       replaced by `-`, keeping the full version valid SemVer and the
#       generated literals valid C++.
#   `nova_git_hash(<out_var> [WORKING_DIRECTORY <dir>])`
#       Short git commit hash of the current checkout.
#   `nova_git_branch(<out_var> [WORKING_DIRECTORY <dir>])`
#       Current branch name (e.g. "main").
#   `nova_git_dirty(<out_var> [WORKING_DIRECTORY <dir>])`
#       "dirty" if the working tree has uncommitted changes, "clean" otherwise.
#
# The working directory defaults to `CMAKE_CURRENT_SOURCE_DIR` of the caller.
# The git values fall back to "unknown" when git or the repository itself
# isn't available (e.g. a source tarball or a Conan cache build).
#
# The values are evaluated at configure time.

function(_nova_git_run out_var exit_var)
    cmake_parse_arguments(PARSE_ARGV 2 arg "" "WORKING_DIRECTORY" "COMMAND")
    if(NOT arg_WORKING_DIRECTORY)
        set(arg_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    find_package(Git QUIET)
    if(NOT Git_FOUND)
        set(${exit_var} 1 PARENT_SCOPE)
        return()
    endif()

    execute_process(
        COMMAND "${GIT_EXECUTABLE}" ${arg_COMMAND}
        WORKING_DIRECTORY "${arg_WORKING_DIRECTORY}"
        OUTPUT_VARIABLE result
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE exit_code
    )

    set(${out_var} "${result}" PARENT_SCOPE)
    set(${exit_var} "${exit_code}" PARENT_SCOPE)
endfunction()

function(nova_git_hash out_var)
    _nova_git_run(result exit_code ${ARGN} COMMAND rev-parse --short HEAD)

    set(hash "unknown")
    if(exit_code EQUAL 0 AND NOT result STREQUAL "")
        set(hash "${result}")
    endif()

    set(${out_var} "${hash}" PARENT_SCOPE)
endfunction()

function(nova_git_branch out_var)
    _nova_git_run(result exit_code ${ARGN} COMMAND rev-parse --abbrev-ref HEAD)

    set(branch "unknown")
    if(exit_code EQUAL 0 AND NOT result STREQUAL "")
        set(branch "${result}")
    endif()

    set(${out_var} "${branch}" PARENT_SCOPE)
endfunction()

function(nova_git_dirty out_var)
    _nova_git_run(result exit_code ${ARGN} COMMAND status --porcelain)

    set(dirty "unknown")
    if(exit_code EQUAL 0)
        if(result STREQUAL "")
            set(dirty "clean")
        else()
            set(dirty "dirty")
        endif()
    endif()

    set(${out_var} "${dirty}" PARENT_SCOPE)
endfunction()

function(nova_target_build_info target)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "NAMESPACE;VERSION;WORKING_DIRECTORY;HASH;BRANCH;DIRTY" "")
    if(NOT arg_NAMESPACE)
        message(FATAL_ERROR "nova_target_build_info(${target}): NAMESPACE is required")
    endif()

    if(NOT arg_VERSION)
        set(arg_VERSION "${PROJECT_VERSION}")
    endif()
    if(NOT arg_VERSION MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
        message(FATAL_ERROR
            "nova_target_build_info(${target}): version `${arg_VERSION}` is not `<major>.<minor>.<patch>`, "
            "set it in `project()` or pass VERSION"
        )
    endif()
    set(NOVA_BUILD_INFO_VERSION "${arg_VERSION}")
    set(NOVA_BUILD_INFO_VERSION_MAJOR "${CMAKE_MATCH_1}")
    set(NOVA_BUILD_INFO_VERSION_MINOR "${CMAKE_MATCH_2}")
    set(NOVA_BUILD_INFO_VERSION_PATCH "${CMAKE_MATCH_3}")

    set(git_args)
    if(arg_WORKING_DIRECTORY)
        set(git_args WORKING_DIRECTORY "${arg_WORKING_DIRECTORY}")
    endif()

    if(NOT arg_HASH)
        nova_git_hash(arg_HASH ${git_args})
    endif()
    if(NOT arg_BRANCH)
        nova_git_branch(arg_BRANCH ${git_args})
    endif()
    if(NOT arg_DIRTY)
        nova_git_dirty(arg_DIRTY ${git_args})
    endif()

    string(REGEX REPLACE "[^0-9A-Za-z.-]" "-" arg_HASH "${arg_HASH}")
    string(REGEX REPLACE "[^0-9A-Za-z.-]" "-" arg_BRANCH "${arg_BRANCH}")

    set(NOVA_BUILD_INFO_NAMESPACE "${arg_NAMESPACE}")
    set(NOVA_BUILD_INFO_HEADER "${target}/gen/build_info.hpp")
    set(NOVA_BUILD_INFO_HASH "${arg_HASH}")
    set(NOVA_BUILD_INFO_BRANCH "${arg_BRANCH}")
    set(NOVA_BUILD_INFO_FULL_VERSION "${arg_VERSION}+${arg_HASH}.${arg_BRANCH}")
    if(arg_DIRTY STREQUAL "dirty")
        set(NOVA_BUILD_INFO_IS_DIRTY "true")
        string(APPEND NOVA_BUILD_INFO_FULL_VERSION ".dirty")
    else()
        set(NOVA_BUILD_INFO_IS_DIRTY "false")
    endif()

    # Include root per target: a target cannot include another one's header.
    set(include_root "${CMAKE_CURRENT_BINARY_DIR}/nova_build_info/${target}")
    set(generated_dir "${include_root}/${target}/gen")
    configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/novaBuildInfo.hpp.in" "${generated_dir}/build_info.hpp" @ONLY)
    configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/novaBuildInfo.cpp.in" "${generated_dir}/build_info.cpp" @ONLY)

    target_sources(${target} PRIVATE "${generated_dir}/build_info.cpp")
    target_include_directories(${target} PRIVATE "$<BUILD_INTERFACE:${include_root}>")

    message(STATUS "Build info of ${target}: ${NOVA_BUILD_INFO_FULL_VERSION}")
endfunction()
