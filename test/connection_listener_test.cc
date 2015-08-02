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

#include <functional>
#include <memory>

#include <gtest/gtest.h>

#include "event_base_test.h"
#include "test_util.h"
#include "wte/connection_listener.h"

namespace wte {

class ConnectionListenerTest : public EventBaseTest {
public:
    ConnectionListenerTest() : accept_count_(0), error_count_(0) { }
    ~ConnectionListenerTest() { }

protected:
    template<typename... Params>
    std::shared_ptr<ConnectionListener> mkListener(Params... params) {
        return mkConnectionListener(std::forward<Params>(params)...);
    }

    std::function<void(int)> mkAccept() {
        return [this](int fd) -> void { acceptCallback(fd); };
    }

    std::function<void(std::exception const& e)> mkError() {
        return [this](std::exception const& e) -> void { errorCallback(e); };
    }

    void acceptCallback(int) {
        ++accept_count_;
    }

    void errorCallback(std::exception const&) {
        ++error_count_;
    }

    std::atomic<int> accept_count_;
    std::atomic<int> error_count_;
};

TEST_F(ConnectionListenerTest, EphemeralPortSelectedOnBindZero) {
    auto listener = mkListener(base, mkAccept(), mkError());
    listener->bind(0);
    ASSERT_GT(listener->port(), 0U);
}

TEST_F(ConnectionListenerTest, JankyConnectTestXXXReplace) {
    auto listener = mkListener(base, mkAccept(), mkError());
    listener->bind(0);
    listener->listen(128);
    listener->startAccepting();

    int sock = connectOrThrow(listener.get());
    ASSERT_NE(-1, sock);
    xclose(sock);

    base->loop(EventBase::LoopMode::ONCE);

    ASSERT_EQ(1, accept_count_);
    ASSERT_EQ(0, error_count_);
}

TEST_F(ConnectionListenerTest, BindToBadIpThrows) {
    auto listener = mkListener(base, mkAccept(), mkError());
    ASSERT_THROW({ listener->bind("not.an.ip", 0); }, std::runtime_error);
}

} // wte namespace
