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

#ifndef WTE_BLOCKING_STREAM_H_
#define WTE_BLOCKING_STREAM_H_

#include <cstdlib>

#include "wte/porting.h"
#include "wte/stream.h"

namespace wte {

class EventBase;

/** Read/write stream with blocking operations. */
class WTE_SYM BlockingStream {
public:
    /**
     * Construct a blocking stream wrapper around an existing descriptor.
     *
     * @param fd the file descriptor
     * @param auto_close whether to close the descriptor on destruction
     */
    explicit BlockingStream(int fd, bool auto_close);

    ~BlockingStream();

    /**
     * Writes the buffer into the stream, blocking if necessary.
     *
     * @param buf the buffer
     * @param size the buffer length
     * @throws on error
     */
    void write(const char *buf, size_t size);

    /**
     * Read up to the requested size, blocking if necessary.
     *
     * May return short reads on EOF.
     *
     * @param buf the buffer
     * @param size the buffer length
     * @return the number of bytes read
     * @throws on error
     */
    int64_t read(char *buf, size_t size);
private:
    EventBase *base_;
    Stream *stream_;
    bool close_;
};

} // wte namespace

#endif // WTE_BLOCKING_STREAM_H_
