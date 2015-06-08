/*
 * Copyright © 2015 Nathan Rosenblum <flander@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "event_base_test.h"
#include "wte/event_handler.h"

namespace wte {

class EventHandlerTest : public EventBaseTest {
public:
    class TestEventHandler final : public EventHandler {
    public:
        explicit TestEventHandler(int fd) : EventHandler(fd),
            last_event(What::NONE) { }
        void ready(What event) noexcept override {
            last_event = event;
        }
        What last_event;
    };
};

TEST_F(EventHandlerTest, WriteEventsPostImmediately) {
    TestEventHandler handler(fds[0]);
    base->registerHandler(&handler, What::WRITE);
    ASSERT_EQ(What::NONE, handler.last_event);

    // Loop once
    base->loop(EventBase::LoopMode::ONCE);

    // Writeable event fired
    ASSERT_EQ(What::WRITE, handler.last_event);
}

TEST_F(EventHandlerTest, ReadEventsPostWhenAvailable) {
    TestEventHandler handler(fds[0]);
    base->registerHandler(&handler, What::READ);

    char buf[1] = {'A'};
    ASSERT_EQ(1, write(fds[1], buf, sizeof(buf)));

    // Loop once
    base->loop(EventBase::LoopMode::ONCE);

    // Readable event fired
    ASSERT_EQ(What::READ, handler.last_event);
}

TEST_F(EventHandlerTest, UnregisteredEventsNotRaised) {
    TestEventHandler handler(fds[0]);
    base->registerHandler(&handler, What::READ);

    char buf[1] = {'A'};
    ASSERT_EQ(1, write(fds[1], buf, sizeof(buf)));

    handler.unregister();

    // Loop until empty (starts empty)
    base->loop(EventBase::LoopMode::UNTIL_EMPTY);

    ASSERT_EQ(What::NONE, handler.last_event);
}

} // wte namespace
