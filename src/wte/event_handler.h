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

#ifndef WTE_EVENT_HANDLER_H_
#define WTE_EVENT_HANDLER_H_

#include "wte/porting.h"
#include "wte/what.h"

namespace wte {

class EventBase;
class EventHandlerImpl;

class EventHandler {
public:
    explicit EventHandler(int fd);
    virtual ~EventHandler();

    /**
     * Callback invoked when the handler is ready.
     *
     * This method is invoked in the context of the event loop thread;
     * implementors should be conscientous about blocking or running
     * compute-intensive operations if the loop thread is shared.
     *
     * Must not throw.
     */
    virtual void ready(What event) NOEXCEPT = 0;

    /**
     * Unregister this event from its base.
     *
     * Idempotent. May only be invoked in the event base thread.
     */
    void unregister();

    /** @return the event base on which this thread is registered, or null. */
    EventBase* base();

    /** @return whether this handler is registered. */
    bool registered();

    /** @return the events that this handler is watching (if registered) .*/
    What watched();

    /** @return the watched file descriptor. */
    int fd() { return fd_; }

    /**
     * Change the file descriptor. It is an error to change the descriptor
     * for a registered handler.
     */
    void setFd(int fd);
private:
    int fd_;
    EventHandlerImpl *impl_;
    friend class EventHandlerImpl;
};

} // wte namespace

#endif // WTE_EVENT_HANDLER_H_
