TODO
====

TODO todo, _todo_. Todo?

![move along](move_along.jpg)

## Building

    mkdir build
    cd build
    cmake ..
    make

If libevent is installed in an obscure location (e.g. on Windows), you may
need to amend the `CMAKE_PREFIX_PATH` variable to help it find the headers and
libraries, e.g.

    cmake -DCMAKE_PREFIX_PATH=%HOME%\libevent

## License

Copyright © 2015 Nathan Rosenblum <flander@gmail.com>

Licensed under the MIT License.

## References

TODO!
