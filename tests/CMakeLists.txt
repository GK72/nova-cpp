add_executable(basic_tests testcases_basics.cpp)
add_executable(timer_tests testcases_timer.cpp)

target_include_directories(basic_tests PRIVATE ${CMAKE_HOME_DIRECTORY}/src)
target_include_directories(timer_tests PRIVATE ${CMAKE_HOME_DIRECTORY}/src)
