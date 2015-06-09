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

BlockingStream::BlockingStream(int fd, bool auto_close)
    : base_(mkEventBase()), stream_(wrapFd(base_, fd)), close_(auto_close) { }

BlockingStream::~BlockingStream() {
    if (close_) {
        // TODO: close
    }
    delete stream_;
    delete base_;
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

    virtual void available(const char *buf, size_t size) override {
        memcpy(buffer_ + nread_, buf, std::min(size, size_ - nread_));
        nread_ += size;
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

void BlockingStream::write(const char *buf, size_t size) {
    BlockingWriteCallback cb;
    stream_->write(buf, size, &cb);
    base_->loop(EventBase::LoopMode::UNTIL_EMPTY);
    if (cb.error_) {
        throw cb.error_.value();
    }
    assert(cb.complete_);
}

int64_t BlockingStream::read(char *buf, size_t size) {
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

} // wte namespace
