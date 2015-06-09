project(tests CXX)

# Set includes
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/test
    ${gtest_INCLUDE_DIRS}
)

add_executable(echo-server
    echo-server.cc
)

target_link_libraries(echo-server
    pthread
    wte
)
