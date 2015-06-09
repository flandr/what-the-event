project(what-the-event-library CXX)

# Define the public include directories
set(WhatTheEvent_PUBLIC_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/wte
    CACHE INTERNAL "what-the-event public include directories"
)

# The shared library target
set(WhatTheEvent_SHARED_LIBRARY
    wte
    CACHE INTERNAL "what-the-event shared library"
)

# Source translation units
set(libwte_SRCS
    buffer.cc
    event_handler.cc
    libevent_connection_listener.cc
    libevent_event_base.cc
    libevent_event_handler.cc
    stream.cc
)

# Set the include directories
include_directories(
    ${WhatTheEvent_PUBLIC_INCLUDE_DIRS}
    .
)

# Main library targets
add_library(${WhatTheEvent_SHARED_LIBRARY} SHARED ${libwte_SRCS})

# Link dependencies
target_link_libraries(${WhatTheEvent_SHARED_LIBRARY}
    ${LibEvent_LIBRARY}
    ${LibEvent_PTHREADS_LIBRARY}
    ${EXTRA_LIBS}
)

# Installation
install(TARGETS
    ${WhatTheEvent_SHARED_LIBRARY}
    DESTINATION lib
    EXPORT wte-targets)
install(EXPORT wte-targets DESTINATION lib)

install(DIRECTORY
    ${CMAKE_SOURCE_DIR}/wte
    DESTINATION include/wte
    FILES_MATCHING PATTERN "*.h"
)
