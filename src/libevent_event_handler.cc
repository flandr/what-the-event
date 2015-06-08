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

#include "libevent_event_handler.h"
#include "wte/event_base.h"

namespace wte {

What LibeventEventHandler::watched() {
    if (!registered_) {
        return What::NONE;
    }
    return fromFlags(event_get_events(&event_));
}

bool LibeventEventHandler::registered() {
    return registered_;
}

namespace {
bool ALL_BITS_SET(int16_t value, int16_t bits) {
    return bits == (value & bits);
}
} // unnamed namespace

What fromFlags(int16_t flags) {
    if (ALL_BITS_SET(flags, EV_READ | EV_WRITE)) {
        return What::READ_WRITE;
    } else if (ALL_BITS_SET(flags, EV_READ)) {
        return What::READ;
    } else if (ALL_BITS_SET(flags, EV_WRITE)) {
        return What::WRITE;
    }
    return What::NONE;
}


} // wte namespace
