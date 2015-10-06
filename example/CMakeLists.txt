project(tests CXX)

# Set includes
include_directories(
    ${WhatTheEvent_PUBLIC_INCLUDE_DIRS}
    ${gtest_INCLUDE_DIRS}
)

add_executable(echo-server
    echo-server.cc
)

target_link_libraries(echo-server
    ${WhatTheEvent_SHARED_LIBRARY}
)

if(NOT WIN32)
    target_link_libraries(echo-server pthread)
endif(NOT WIN32)
