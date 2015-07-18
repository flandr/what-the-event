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

#ifndef WTE_WHAT_H_
#define WTE_WHAT_H_

#include "wte/porting.h"

namespace wte {

/** Event types. */
enum class WTE_SYM What {
    NONE,
    READ,
    WRITE,
    READ_WRITE,
};

/** @return whether the event indicates a read. */
inline bool isRead(What what) {
    return what == What::READ || what == What::READ_WRITE;
}

/** @return whether the event indicates a write. */
inline bool isWrite(What what) {
    return what == What::WRITE || what == What::READ_WRITE;
}

/** @return `what` | WRITE. */
inline What ensureWrite(What what) {
    switch (what) {
    case What::READ:
        return What::READ_WRITE;
    default:
        return What::WRITE;
    }
}

/** @return `what` | READ. */
inline What ensureRead(What what) {
    switch (what) {
    case What::WRITE:
        return What::READ_WRITE;
    default:
        return What::READ;
    }
}

inline What removeWrite(What what) {
    switch (what) {
        case What::READ_WRITE:
        case What::READ:
            return What::READ;
        default:
            return What::NONE;
    }
}

} // wte namespace

#endif // WTE_WHAT_H_
