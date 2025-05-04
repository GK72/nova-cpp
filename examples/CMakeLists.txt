add_executable(shielded shielded.cc)
target_link_libraries(shielded PRIVATE nova nova-deps)

add_executable(nova-version nova-version.cc)
target_link_libraries(nova-version PRIVATE nova nova-deps)
