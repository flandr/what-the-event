project(what-the-event CXX)

# Required CMake version
cmake_minimum_required(VERSION 2.8)

# Module search path
set(CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/cmake
)

# Some CMake configuration
set(CMAKE_COLOR_MAKEFILE ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE ON)

# Default to release builds w/ debug info
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif (NOT CMAKE_BUILD_TYPE)

# Add third-party dependencies
include(ExternalProject)
include(External_libevent)
include(External_gtest)

# Use C++11
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_subdirectory(example)
add_subdirectory(src)
add_subdirectory(test)
