Event loops, simplified
=======================

Event-driven IO handlers are an efficient mechanism for implementing network
servers and clients. Excellent libraries like [libevent](http://libevent.org/)
and [libuv](https://github.com/libuv/libuv) provide portable event loop
implementations that abstract away platform-specific details of monitoring
sockets or file descriptors for events. These APIs, however, have considerable
complexity can can be awkward to use in modern C++ programs. What The Event?
offers an alternative, providing a C++ asynchronous IO interface aimed at
multithreaded server and client applications.

## Feature highlights

What The Event? wraps a subset of the functionality of
[libevent](http://libevent.org) and exposes similar concepts:

 - Manual or continuously driven [event loops](src/wte/event_base.h)
 - Buffered [asynchronous stream IO](src/wte/stream.h)
 - Convenience [blocking interfaces](src/wte/blocking_stream.h)
 - Socket [listener](stc/wte/connection_listener.h) for server applications
 - Arbitrary deferred task execution
 - Safe for use in multithreaded programs
 - Cross-platform (Windows, OS X, Linux) support

The [echo server](example/echo-server.cc) demonstrates a small server
application based on this library.

WTE is a work in progress; refer to the [TODO](TODO.md) list for future
functionality.

## Supported platforms

What The Event? supports Linux, OS X, and Windows 64-bit platforms. It could
in principal be extended to any combination of operating system and
architecture that [libevent](http://libevent.org/) supports.

## Building

The WTE build system is [CMake](http://www.cmake.org/)-based, so an
installation of CMake 2.8+ is required. On Linux systems, we recommend using a
system-provided installation of [libevent](http://libevent.org/) if possible;
on Windows this dependency will be resolved automatically.

WTE uses C++11 features and requires a supported compiler. It is known to work
with at least the following compilers:

 - GCC 4.8
 - Apple LLVM 6.1.0 (Xcode 6.4)
 - MSVC 12 (Visual Studio 2013)

### Linux and OSX

Building on Linux and OSX is straightforward:

    mkdir build
    cd build
    cmake ..
    make

To select a specific compiler, pass the full path to CMake via `cmake
-DCMAKE_CXX_COMPILER=<path> ..`.

## Windows

The WTE library requires MSVC 12+; depending on your build environment you may
need to define a target environment by passing `-G <target>` to `cmake`:

    mkdir build
    cd build
    cmake -G "Visual Studio 12"
    msbuild what-the-event.sln

By default both dynamic and static libraries will be built. Note that if you
want to link `wte_s.lib` statically, you will also need to add the
`libevent.lib` static library to the link command.

## Related projects

What The Event? uses the excellent [libevent](http://libevent.org/) library
for cross-platform monitoring of IO events.

[Facebook's Folly library](https://github.com/facebook/folly) includes an
asynchronous IO component that is similar in several respects to WTE
(libevent-based, exposes stream-like interfaces for reading and writing).
Facebook's async io support may be a better choice if your target platforms
are limited to Linux and OS X or if you are using other components of the
Folly library.

## License

Copyright © 2015 Nathan Rosenblum <flander@gmail.com>

Licensed under the MIT License.
