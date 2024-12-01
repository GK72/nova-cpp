add_library(nova-tcp tcp.cc)
target_link_libraries(nova-tcp PUBLIC
    nova-headers

    boost::boost
    spdlog::spdlog_header_only
)

code_analysis(nova-tcp PRIVATE)
