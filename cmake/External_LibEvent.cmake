find_package(LibEvent)

if(LIBEVENT_FOUND)
    include_directories(${LibEvent_INCLUDE_DIR})
endif()
