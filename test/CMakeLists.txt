project(tests CXX)

# Set includes
include_directories(
    ${WhatTheEvent_PUBLIC_INCLUDE_DIRS}
    ${gtest_INCLUDE_DIRS}
)

IF(WIN32)
    # Linking statically; pretend we're exporting
    add_definitions("-DEXPORTING")
ENDIF(WIN32)

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
)

if(NOT WIN32)
    target_link_libraries(test ${WhatTheEvent_SHARED_LIBRARY})
else(NOT WIN32)
    target_link_libraries(test ${WhatTheEvent_STATIC_LIBRARY} ${LibEvent_LIBRARY} ws2_32)
endif(NOT WIN32)

if(NOT WIN32)
    target_link_libraries(test pthread)
endif(NOT WIN32)
