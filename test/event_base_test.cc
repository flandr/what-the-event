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

#include <thread>

#include "event_base_test.h"
#include "wte/event_base.h"
#include "wte/event_handler.h"

namespace wte {

TEST_F(EventBaseTest, StopTerminatesLoop) {
    // loop() waits for active events, of which there are none by default
    std::thread loop([this]() { base->loop(EventBase::LoopMode::ONCE); });
    ASSERT_TRUE(loop.joinable());
    base->stop();
    loop.join();
}

class TestEventHandler final : public EventHandler {
public:
    explicit TestEventHandler(int fd) : EventHandler(fd),
        last_event(What::NONE) { }
    void ready(What event) noexcept override {
        last_event = event;
    }
    What last_event;
};

TEST_F(EventBaseTest, RegistrationStateChanges) {
    TestEventHandler handler(fds[0]);
    ASSERT_FALSE(handler.registered());

    // Registration
    base->registerHandler(&handler, What::READ);
    ASSERT_TRUE(handler.registered());
    ASSERT_EQ(What::READ, handler.watched());

    // Idempotence
    base->registerHandler(&handler, What::READ);
    ASSERT_EQ(What::READ, handler.watched());

    // Re-registration
    base->registerHandler(&handler, What::WRITE);
    ASSERT_EQ(What::WRITE, handler.watched());

    // Unregistration
    base->unregisterHandler(&handler);
    ASSERT_FALSE(handler.registered());
    ASSERT_EQ(What::NONE, handler.watched());

    // Idempotence
    base->unregisterHandler(&handler);
    ASSERT_FALSE(handler.registered());
}

} // wte namespace
