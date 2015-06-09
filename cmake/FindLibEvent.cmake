# Find a libevent installation
# defines LibEevent_FOUND, LibEvent_INCLUDE_DIR, LibEvent_LIBRARIES

FIND_PATH(LibEvent_INCLUDE_DIR NAMES event.h)
FIND_LIBRARY(LibEvent_LIBRARY NAMES event)
FIND_LIBRARY(LibEvent_CORE_LIBRARY NAMES event_core)
FIND_LIBRARY(LibEvent_PTHREADS_LIBRARY NAMES event_pthreads)
FIND_LIBRARY(LibEvent_EXTRA_LIBRARY NAMES event_extra)
FIND_LIBRARY(LibEvent_OPENSSL_LIBRARY NAMES event_openssl)


INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibEvent DEFAULT_MSG LibEvent_LIBRARY LibEvent_INCLUDE_DIR)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibEventPthreads DEFAULT_MSG LibEvent_PTHREADS_LIBRARY LibEvent_INCLUDE_DIR)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibEventCore DEFAULT_MSG LibEvent_CORE_LIBRARY LibEvent_INCLUDE_DIR)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibEventExtra DEFAULT_MSG LibEvent_EXTRA_LIBRARY LibEvent_INCLUDE_DIR)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibEventOpenssl DEFAULT_MSG LibEvent_OPENSSL_LIBRARY LibEvent_INCLUDE_DIR)

MARK_AS_ADVANCED(LibEvent_INCLUDE_DIR LibEvent_LIBRARY LibEvent_PTHREADS_LIBRARY LibEvent_OPENSSL_LIBRARY LibEvent_CORE_LIBRARY LibEvent_EXTRA_LIBRARY)
