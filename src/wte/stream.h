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

#include <cinttypes>
#include <functional>
#include <stdexcept>
#include <memory>

#include "wte/buffer.h"
#include "wte/event_base.h"
#include "wte/porting.h"

namespace wte {

/**
 * Interface for an asynchronous data stream.
 */
class Stream {
public:
    class WTE_SYM Deleter {
    public:
        void operator()(Stream *);
    };

    /**
     * Allocate an return an unconnected stream.
     *
     * Streams created by this method must be established via a call to
     * `connect` before they can be used. Failure to do so will cause
     * an exception to be thrown by most interfaces, except where noted.
     *
     * @param base the event base for stream IO
     * @return an unconnected stream
     */
    WTE_SYM static std::unique_ptr<Stream, Deleter> create(
        std::shared_ptr<EventBase> base);

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
         * The stream assumes that the data are consumed by this method; i.e.,
         * the callback is _edge triggered_ and will not fire again if the
         * callback fails to consume data.
         *
         * @param buffer the available data buffer
         */
        virtual void available(Buffer *buffer) = 0;

        /** Invoked when an error occurs on the channel. */
        virtual void error(std::runtime_error const&) = 0;

        /** Invoked when the stream has been closed on the other side. */
        virtual void eof() = 0;
    };

    class ConnectCallback {
    public:
        /** Invoked when the connection completes successfully. */
        virtual void complete() = 0;

        /** Invoked when the connection fails. */
        virtual void error(std::runtime_error const&) = 0;
    };

    /**
     * Write a block of data to the stream, with an optional callback
     * to handle success or failure notification.
     *
     * It is the caller's responsibility to ensure that the write callback
     * (if provided) remains live until it is invoked or the stream is closed.
     *
     * May only be invoked on the stream's event base.
     *
     * @param buf the buffer
     * @param size the buffer size
     * @param cb the callback (nullable)
     */
    virtual void write(const char *buf, size_t size, WriteCallback *cb) = 0;

    /**
     * Write a block of data to the stream, with an optional callback
     * to handle success or failure notification.
     *
     * It is the caller's responsibility to ensure that the write callback
     * (if provided) remains live until it is invoked or the stream is closed.
     *
     * May only be invoked on the stream's event base.
     *
     * @param buf the buffer
     * @param cb the callback (nullable)
     */
    virtual void write(Buffer *buf, WriteCallback *cb) = 0;

    /**
     * Starts reading on the stream.
     *
     * The read callback will be continuously invoked as long as there are
     * data available on the stream to read, until the `stopRead` method
     * is invoked.
     *
     * May only be invoked on the stream's event base.
     */
    virtual void startRead(ReadCallback *cb) = 0;

    /**
     * Stop reading the stream.
     *
     * No read callbacks will fire after this method returns.
     *
     * May only be invoked on the stream's event base.
     */
    virtual void stopRead() = 0;

    /**
     * Closes the stream.
     *
     * Invokes the eof callback if a read callback is registered.
     *
     * May only be invoked on the stream's event base.
     */
    virtual void close() = 0;

    /**
     * Connect to the specified ip and port.
     *
     * Invokes the connection callback on success or failure.
     *
     * May only be invoked on the stream's event base.
     *
     * @param ip_addr the target host ip
     * @param port the target host port
     * @param cb the connection callback
     */
    virtual void connect(std::string const& ip_addr, int16_t port,
        ConnectCallback *cb) = 0;
};

// TODO: temporary interface for testing. Must already be connected & set
// to non-blocking mode.
WTE_SYM std::unique_ptr<Stream, Stream::Deleter> wrapFd(
    std::shared_ptr<EventBase> base, int fd);

} // wte namespace

#endif // WTE_STREAM_H_
