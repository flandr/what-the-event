project(what-the-event CXX)

# Required CMake version
cmake_minimum_required(VERSION 2.8)

set (WhatTheEvent_MAJOR_VERSION 0)
set (WhatTheEvent_MINOR_VERSION 1)
set (WhatTheEvent_PATCH_VERSION 0)
set (WhatTheEvent_VERSION
    ${WhatTheEvent_MAJOR_VERSION}.${WhatTheEvent_MINOR_VERSION}.${WhatTheEvent_PATCH_VERSION})

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
include(External_LibEvent)

# Use C++11
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_subdirectory(example)
add_subdirectory(src)

option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
    include(External_gtest)
    add_subdirectory(test)
endif()

# Packaging
set (CPACK_PACKAGE_NAME libwte-dev)
set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "C++-friendly wrapper library for libevent")
set (CPACK_PACKAGE_VERSION_MAJOR ${WhatTheEvent_MAJOR_VERSION})
set (CPACK_PACKAGE_VERSION_MINOR ${WhatTheEvent_MINOR_VERSION})
set (CPACK_PACKAGE_VERSION_PATCH ${WhatTheEvent_PATCH_VERSION})
set (CPACK_PACKAGE_VERSION ${WhatTheEvent_VERSION})

set (CPACK_GENERATOR "DEB")
set (CPACK_DEBIAN_PACKAGE_MAINTAINER "Nate Rosenblum <flander@gmail.com>")
set (CPACK_DEBIAN_PACKAGE_DEPENDS "libevent-2.0-5")

include (CPack)
