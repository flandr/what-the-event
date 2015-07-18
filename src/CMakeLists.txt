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

# The shared library target
set(WhatTheEvent_STATIC_LIBRARY
    wte_s
    CACHE INTERNAL "what-the-event shared library"
)

# Source translation units
set(libwte_SRCS
    blocking_stream.cc
    buffer.cc
    event_handler.cc
    libevent_connection_listener.cc
    libevent_event_base.cc
    libevent_event_handler.cc
    stream.cc
    timeout.cc
)

# Set the include directories
include_directories(
    ${WhatTheEvent_PUBLIC_INCLUDE_DIRS}
    .
)

# Export symbols, when relevant
add_definitions("-DEXPORTING")

# Main library targets
add_library(${WhatTheEvent_SHARED_LIBRARY} SHARED ${libwte_SRCS})
add_library(${WhatTheEvent_STATIC_LIBRARY} STATIC ${libwte_SRCS})
if(NOT WIN32)
    # Windows needs separate names, but other platforms do not
    set_target_properties(wte_s PROPERTIES OUTPUT_NAME wte)
endif(NOT WIN32)

add_dependencies(${WhatTheEvent_SHARED_LIBRARY} libevent_ext)
add_dependencies(${WhatTheEvent_STATIC_LIBRARY} libevent_ext)

if(WIN32)
   set(EXTRA_LIBS ws2_32)
endif(WIN32)

# Link dependencies
target_link_libraries(${WhatTheEvent_SHARED_LIBRARY}
    ${LibEvent_LIBRARY}
    ${EXTRA_LIBS}
)

# Installation
install(TARGETS
    ${WhatTheEvent_SHARED_LIBRARY}
    ${WhatTheEvent_STATIC_LIBRARY}
    DESTINATION lib
    EXPORT wte-targets)
install(EXPORT wte-targets DESTINATION lib)

install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/wte
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)
