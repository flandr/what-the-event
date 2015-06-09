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

#ifndef WTE_STREAM_H_
#define WTE_STREAM_H_

#include <stdexcept>
#include <functional>

#include "wte/event_base.h"

namespace wte {

/**
 * Interface for an asynchronous data stream.
 */
class Stream {
public:
    virtual ~Stream() { }

    /** Write callback interface. */
    class WriteCallback {
    public:
        /** Invoked when a write request completes successfully. */
        virtual void complete(Stream *) = 0;

        /** Invoked when a write request encounters an error. */
        virtual void error(std::runtime_error const&) = 0;
    };

    class ReadCallback {
    public:
        /**
         * Invoked when new data are available for reading.
         *
         * The stream assumes that the data are consumed by this method.
         * TODO: consider peekable methods for copy-avoidance.
         */
        virtual void available(const char *, size_t) = 0;

        /** Invoked when an error occurs on the channel. */
        virtual void error(std::runtime_error const&) = 0;

        /** Invoked when the stream has been closed on the other side. */
        virtual void eof() = 0;
    };

    /**
     * Write a block of data to the stream, with an optional callback
     * to handle success or failure notification.
     *
     * It is the caller's responsibility to ensure that the write callback
     * (if provided) remains live until it is invoked or the stream is closed.
     *
     * @param buf the buffer
     * @param size the buffer size
     * @param cb the callback (nullable)
     */
    virtual void write(const char *buf, size_t size, WriteCallback *cb) = 0;

    /**
     * Starts reading on the stream.
     *
     * The read callback will be continuously invoked as long as there are
     * data available on the stream to read, until the `stopRead` method
     * is invoked.
     */
    virtual void startRead(ReadCallback *cb) = 0;

    /**
     * Stop reading the stream.
     *
     * No read callbacks will fire after this method returns.
     */
    virtual void stopRead() = 0;

    /**
     * Closes the stream.
     *
     * Invokes the eof callback if a read callback is registered.
     */
    virtual void close() = 0;
};

// TODO: temporary interface for testing. Must already be connected & set
// to non-blocking mode.
Stream *wrapFd(EventBase *base, int fd);

} // wte namespace

#endif // WTE_STREAM_H_
