project(tests CXX)

# Set includes
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/test
    ${gtest_INCLUDE_DIRS}
)

add_executable(test
    blocking_stream_test.cc
    buffer_test.cc
    driver.cc
    connection_listener_test.cc
    event_base_test.cc
    event_handler_test.cc
    mpsc_queue_test.cc
    stream_test.cc
    test_util.cc
    timeout_test.cc
    optional_test.cc
)

target_link_libraries(test
    gtest
    wte
)

if(NOT WIN32)
    target_link_libraries(test pthread)
endif(NOT WIN32)
