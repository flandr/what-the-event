if(NOT WIN32)
    find_package(LibEvent)
    if(LIBEVENT_FOUND)
        include_directories(${LibEvent_INCLUDE_DIR})
    endif()
else(NOT WIN32)
    ExternalProject_Add(libevent_ext
        URL http://github.com/libevent/libevent/releases/download/release-2.0.22-stable/libevent-2.0.22-stable.tar.gz 
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/libevent
        CONFIGURE_COMMAND ""
        BUILD_COMMAND nmake -f Makefile.nmake static_libs
        INSTALL_COMMAND ""
        BUILD_IN_SOURCE 1
    )

    # Ugly and horrible but not worth fighting about
    set(LibEvent_BASE ${CMAKE_CURRENT_BINARY_DIR}/libevent/src/libevent_ext)
    set(LibEvent_LIBRARY ${LibEvent_BASE}/libevent.lib)
    include_directories(${LibEvent_BASE}/include ${LibEvent_BASE}/win32-code)
endif(NOT WIN32)

