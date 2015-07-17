find_package(LibEvent)

if(LIBEVENT_FOUND)
    include_directories(${LibEvent_INCLUDE_DIR})
    if(WIN32)
        include_directories(${LibEvent_WIN_INCLUDE_DIR})
    endif(WIN32)
endif()
