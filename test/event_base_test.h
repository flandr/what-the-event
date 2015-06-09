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

#ifndef TEST_EVENT_BASE_TEST_H_
#define TEST_EVENT_BASE_TEST_H_

#include <event2/util.h>
#include <gtest/gtest.h>

#include "wte/event_base.h"

namespace wte {

class EventBaseTest : public ::testing::Test {
public:
    EventBaseTest() : base(mkEventBase()) {
#if defined(_WIN32)
        int rc = evutil_socketpair(AF_INET, SOCK_STREAM, 0, fds);
#else
        int rc = evutil_socketpair(AF_LOCAL, SOCK_STREAM, 0, fds);
#endif
        if (-1 == rc) {
            throw std::runtime_error("Failed to allocate socket pair");
        }
        if (-1 == evutil_make_socket_nonblocking(fds[0])) {
            throw std::runtime_error("Failed to make socket nonblocking");
        }
        if (-1 == evutil_make_socket_nonblocking(fds[1])) {
            throw std::runtime_error("Failed to make socket nonblocking");
        }
    }
    ~EventBaseTest() {
        closepipe<0>();
        closepipe<1>();
        delete base;
    }

    // Trading additional code emitted for avoiding parameter type screw-ups :P
    template<int Idx>
    void closepipe() {
        if (fds[Idx] == -1) {
            return;
        }
        close(fds[Idx]);
        fds[Idx] = -1;
    }
protected:
    evutil_socket_t fds[2]; // Directly depending on libevent utils
    EventBase *base;
};

} // namespace wte

#endif // TEST_EVENT_BASE_TEST_H_
