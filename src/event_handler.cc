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

#include "wte/event_handler.h"

#include <cassert>

#include "event_handler_impl.h"
#include "wte/event_base.h"

namespace wte {

EventHandler::EventHandler(int fd) : fd_(fd), impl_(nullptr) { }

EventHandler::~EventHandler() {
    delete impl_;
}

void EventHandler::unregister() {
    assert(base() != nullptr);
    base()->unregisterHandler(this);
}

EventBase* EventHandler::base() {
    if (!impl_) {
        return nullptr;
    }
    return impl_->base();
}

bool EventHandler::registered() {
    if (!impl_) {
        return false;
    }
    return impl_->registered();
}

What EventHandler::watched() {
    if (!impl_) {
        return What::NONE;
    }
    return impl_->watched();
}

} // wte namespace
