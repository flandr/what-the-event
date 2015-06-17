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

#ifndef WTE_EVENT_BASE_H_
#define WTE_EVENT_BASE_H_

#include <functional>

#include "wte/what.h"

namespace wte {

class EventHandler;

class EventBase {
public:
    enum class LoopMode {
        /** Process active events and return. */
        ONCE,
        /** Run the loop until no handlers are registered. */
        UNTIL_EMPTY,
        /** Run the loop forever. */
        FOREVER,
    };

    /**
     * Run the event loop in the specified mode.
     *
     * @param mode the loop mode
     * @throws an exception on error.
     */
    virtual void loop(LoopMode mode) = 0;

    /**
     * Stop the event loop.
     *
     * This method can safely be invoked from any thread.
     */
    virtual void stop() = 0;

    /**
     * Register an event handler on this base.
     *
     * If the handler is already registered on this base, updates the
     * events that it will handle.
     *
     * The handler must not be registered on another base.
     *
     * May only be invoked on the even loop thread.
     */
    virtual void registerHandler(EventHandler *handler, What events) = 0;

    /**
     * Unregister the event handler.
     *
     * May only be invoked on the event loop thread.
     */
    virtual void unregisterHandler(EventHandler *handler) = 0;

    /**
     * Enqueue an operation to run on this event base.
     *
     * This method may be invoked on any thread, including the thread
     * currently driving this event loop. In the latter case, the operation
     * is invoked immediately, without enqueuing.
     *
     * @return false on error, otherwise true
     */
    virtual bool runOnEventLoop(std::function<void(void)> const& op) = 0;

    /**
     * Enqeue an operation to run on the event base and wait for completion.
     *
     * Like `runOnEventLoop`, this executes immediately if invoked from
     * the event loop thread.
     *
     * @return false on error, otherwise true
     */
    virtual bool runOnEventLoopAndWait(std::function<void(void)> const& op) = 0;

    virtual ~EventBase() { }
};

/** @return a new event base. */
EventBase *mkEventBase();

} // wte namespace

#endif // WTE_EVENT_BASE_H_
