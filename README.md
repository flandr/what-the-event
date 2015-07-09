TODO
====

TODO todo, _todo_. Todo?

![move along](move_along.jpg)

## Executors

The primary purpose of What The Event? is to support simple event-driven IO.
It also happens that the primitives for such support (isolated event driver
loops, efficient inter-thread communication) can be used to provide some
concrete implementations of the _Executor_ interface proposed in the
[N3785](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3785.pdf) and
[N4143](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4143.pdf) C++
working group drafts. An executor defines a single API method `void
execute(Func &&)` that causes the task in `Func` to be executed at some time
in the future in some thread of execution, with the specifics defined by the
executor type. N4143 defines several executor types, of which WTE implements
`loop_executor` and `serial_executor`.

### Loop executor

The _loop executor_ queues tasks but does not execute them until an
execution-driving method has been invoked. _TODO_

### Serial executor

_TODO_

## License

Copyright © 2015 Nathan Rosenblum <flander@gmail.com>

Licensed under the MIT License.

## References

TODO!
