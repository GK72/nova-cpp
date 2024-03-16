add_executable(shielded shielded.cc)
target_link_libraries(shielded PRIVATE nova nova-deps)

add_custom_command(
    TARGET shielded
    COMMAND sudo setcap cap_sys_nice+ep shielded
)
