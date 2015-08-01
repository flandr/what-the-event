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

#ifndef WTE_CONNECTION_LISTENER_H_
#define WTE_CONNECTION_LISTENER_H_

#include <cinttypes>
#include <functional>

#include "wte/event_base.h"
#include "wte/porting.h"

namespace wte {

/**
 * A class that can listen for and accept connections.
 *
 * Callbacks are executed on the event base passed to this constructor.
 * A typical usage pattern on multi-core systems would be to dispatch
 * the returned file descriptor to request handling on another event loop
 * running on another processor, possibly wrapped in an asynchronous
 * `Stream` or `BlockingStream`.
 *
 * The event base passed to the constructor need not be running; needless
 * to say, no connections will be accepted until `bind`, `listen`, and
 * `startAccepting` are invoked, and the event base is driven by its
 * `loop` method.
 */
class WTE_SYM ConnectionListener {
public:
    virtual ~ConnectionListener() { }

    /**
     * Bind the the specified port on all interfaces.
     *
     * @throws on error
     */
    virtual void bind(uint16_t port) = 0;

    /**
     * Bind the the specified port on the specified ip.
     *
     * @throws on error
     */
    virtual void bind(std::string const& ip_addr, uint16_t port) = 0;

    /**
     * Start listening for connections on the bound port, with specified
     * backlog. Invoking `listen` prior to `bind` will throw an exception.
     *
     * @param backlog number of connections to queue at the kernel level
     *                before returning ECONNREFUSED to clients. The
     *                limit for this parameter on most systems is 128.
     *
     * @throws on error
     */
    virtual void listen(int backlog) = 0;

    /**
     * Start accepting connections.
     *
     * @throws on error
     */
    virtual void startAccepting() = 0;

    /**
     * Stop accepting connections.
     *
     * @throws on error
     */
    virtual void stopAccepting() = 0;

    /** @return the bound port. Undefined prior to invoking `bind`. */
    virtual uint16_t port() = 0;
};

/**
 * Construct a connection listener.
 *
 * @param base the event base for the listener
 * @param acceptCallback the callback to be invoked on successful accepts
 * @param errorCallback the callback to be invoked on errors
 * @throws on error
 */
WTE_SYM ConnectionListener* mkConnectionListener(
    std::shared_ptr<EventBase> base,
    std::function<void(int fd)> const& acceptCallback,
    std::function<void(std::exception const&)> errorCallback);

} // wte namespace

#endif // WTE_CONNECTION_LISTENER_H_
