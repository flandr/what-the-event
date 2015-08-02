/*
 * Copyright (©) 2015 Nate Rosenblum
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef SRC_LIBEVENT_CONNECTION_LISTENER_H_
#define SRC_LIBEVENT_CONNECTION_LISTENER_H_

#include "wte/connection_listener.h"

#include "wte/event_handler.h"
#include "wte/porting.h"

namespace wte {

// TODO: other than utility methods used, there's not much libevent-specific
// here. Consider making generic, like Stream.
class LibeventConnectionListener final : public ConnectionListener {
public:
    LibeventConnectionListener(std::shared_ptr<EventBase> loop,
        std::function<void(int)> const& acceptCallback,
        std::function<void(std::exception const&)> errorCallback);
    ~LibeventConnectionListener();

    void bind(uint16_t port) override;
    void bind(std::string const& ip_addr, uint16_t port) override;
    void listen(int backlog) override;
    void startAccepting() override;
    void stopAccepting() override;
    uint16_t port() override { return port_; }
private:
    class AcceptHandler final : public EventHandler {
    public:
        AcceptHandler(LibeventConnectionListener *listener, int fd)
            : EventHandler(fd), listener_(listener) { }
        void ready(What what) NOEXCEPT override;
    private:
        LibeventConnectionListener *listener_;
    };

    std::shared_ptr<EventBase> base_;
    uint16_t port_;
    std::function<void(int)> acceptCallback_;
    std::function<void(std::exception const&)> errorCallback_;
    AcceptHandler handler_;
};

} // wte namespace

#endif // SRC_LIBEVENT_CONNECTION_LISTENER_H_
