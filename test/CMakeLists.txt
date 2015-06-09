project(tests CXX)

# Set includes
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/test
    ${gtest_INCLUDE_DIRS}
)

add_executable(test
    buffer_test.cc
    driver.cc
    connection_listener_test.cc
    event_base_test.cc
    event_handler_test.cc
    stream_test.cc
    test_util.cc
    optional_test.cc
)

target_link_libraries(test
    gtest
    pthread
    wte
)
