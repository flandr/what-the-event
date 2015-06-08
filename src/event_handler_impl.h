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

#ifndef SRC_EVENT_HANDLER_IMPL_H_
#define SRC_EVENT_HANDLER_IMPL_H_

#include <event2/event.h>

#include "wte/event_handler.h"
#include "wte/what.h"

namespace wte {

class EventBase;

class EventHandlerImpl {
public:
    virtual ~EventHandlerImpl() { }
    virtual What watched() = 0;
    virtual EventBase* base() = 0;
    virtual bool registered() = 0;

    static EventHandlerImpl* get(EventHandler *h) { return h->impl_; }
    static void set(EventHandler *h, EventHandlerImpl *impl) {
        h->impl_ = impl;
    }
};

} // wte namespace

#endif // SRC_EVENT_HANDLER_IMPL_H_
