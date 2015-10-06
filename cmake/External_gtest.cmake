set(_source "${CMAKE_SOURCE_DIR}/third_party/gtest")
set(_build "${CMAKE_CURRENT_BINARY_DIR}/gtest")

if (NOT TARGET gtest_ext)

ExternalProject_Add(gtest_ext
    SOURCE_DIR ${_source}
    BINARY_DIR ${_build}
    CMAKE_ARGS -Dgtest_force_shared_crt=ON -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
    # googletest doesn't provide an install target
    INSTALL_COMMAND ""
)

set(gtest_INCLUDE_DIRS "${_source}/include")
link_directories("${_build}")

endif ()
