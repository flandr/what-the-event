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

#include "wte/blocking_stream.h"

#include <algorithm>
#include <cassert>
#include <cstring>

#include "optional.h"
#include "wte/event_base.h"
#include "wte/stream.h"

namespace wte {

class BlockingStreamImpl final : public BlockingStream {
public:
    /**
     * Construct a blocking stream wrapper around an existing descriptor.
     *
     * @param fd the file descriptor
     * @param auto_close whether to close the descriptor on destruction
     */
    BlockingStreamImpl(int fd, bool auto_close);

    ~BlockingStreamImpl();

    /**
     * Writes the buffer into the stream, blocking if necessary.
     *
     * @param buf the buffer
     * @param size the buffer length
     * @throws on error
     */
    void write(const char *buf, size_t size) override;

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
    int64_t read(char *buf, size_t size) override;
private:
    std::shared_ptr<EventBase> base_;
    std::unique_ptr<Stream, Stream::Deleter> stream_;
    bool close_;
};

BlockingStreamImpl::BlockingStreamImpl(int fd, bool auto_close)
    : base_(mkEventBase()), stream_(wrapFd(base_, fd)), close_(auto_close) { }

BlockingStreamImpl::~BlockingStreamImpl() {
    if (close_) {
        // TODO: close
    }
    stream_.reset(nullptr);
    base_.reset();
}

namespace {

class BlockingWriteCallback final : public Stream::WriteCallback {
public:
    void complete(Stream *) {
        complete_ = true;
    }

    void error(std::runtime_error const& e) {
        error_ = Optional<std::runtime_error>(e);
    }

    Optional<std::runtime_error> error_;
    bool complete_ = false;
};

class BlockingReadCallback final : public Stream::ReadCallback {
public:
    explicit BlockingReadCallback(char *buf, size_t expected) {
        buffer_ = buf;
        size_ = expected;
    }

    ~BlockingReadCallback() { }

    virtual void available(Buffer *buf) override {
        size_t nread = 0;
        buf->read(buffer_ + nread_, size_ - nread_, &nread);
        nread_ += nread;
    }

    virtual void error(std::runtime_error const& e) {
        error_ = Optional<std::runtime_error>(e);
    }

    virtual void eof() {
        eof_ = true;
    }

    char *buffer_ = nullptr;
    size_t size_ = 0;
    size_t nread_ = 0;
    bool eof_ = false;
    Optional<std::runtime_error> error_;
};

} // unnamed namespace

void BlockingStreamImpl::write(const char *buf, size_t size) {
    BlockingWriteCallback cb;
    stream_->write(buf, size, &cb);
    base_->loop(EventBase::LoopMode::UNTIL_EMPTY);
    if (cb.error_) {
        throw cb.error_.value();
    }
    assert(cb.complete_);
}

int64_t BlockingStreamImpl::read(char *buf, size_t size) {
    BlockingReadCallback cb(buf, size);
    stream_->startRead(&cb);

    do {
        base_->loop(EventBase::LoopMode::ONCE);
    } while (cb.nread_ < size && !cb.error_ && !cb.eof_);

    if (cb.error_) {
        throw cb.error_.value();
    }

    return cb.nread_;
}

void BlockingStream::Deleter::operator()(BlockingStream *bs) {
    delete bs;
}

std::unique_ptr<BlockingStream, BlockingStream::Deleter>
BlockingStream::create(int fd, bool auto_close) {
    return std::unique_ptr<BlockingStream, Deleter>(
        new BlockingStreamImpl(fd, auto_close), Deleter());
}

} // wte namespace
