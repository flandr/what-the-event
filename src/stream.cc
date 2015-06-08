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

#include "wte/stream.h"

#include <unistd.h>

#include <cassert>

#include "wte/buffer.h"
#include "wte/event_base.h"
#include "wte/event_handler.h"

namespace wte {

class StreamImpl final : public Stream {
public:
    // TODO: temporary fd-based constructor for testing
    StreamImpl(EventBase *base, int fd) : handler_(this, fd),
        base_(base), requests_{nullptr, nullptr} { }
    ~StreamImpl();

    void write(const char *buf, size_t size, WriteCallback *cb) override;
    void startRead(ReadCallback *cb) override { }
    void stopRead() override { }
private:
    void writeHelper();

    class SockHandler final : public EventHandler {
    public:
        SockHandler(StreamImpl *stream, int fd)
            : EventHandler(fd), stream_(stream) { }
        void ready(What what) noexcept override;
    private:
        StreamImpl *stream_;
    };

    // State about a write request (buffer, callback)
    class WriteRequest {
    public:
        WriteRequest(const char *buffer, size_t size, WriteCallback *cb);
        ~WriteRequest();
        Buffer buffer_;
        WriteCallback *callback_;
        WriteRequest *next_;
    };

    SockHandler handler_;
    EventBase *base_;
    struct Requests {
        WriteRequest *head;
        WriteRequest *tail;

        void append(WriteRequest *req) {
            if (!head) {
                assert(!tail);
                head = req;
                tail = req;
            } else {
                tail->next_ = req;
            }
        }

        WriteRequest* consumeFront() {
            if (!head) {
                return nullptr;
            }
            WriteRequest *tmp = head;
            head = head->next_;
            if (!head) {
                tail = nullptr;
            }
            delete tmp;
            return head;
        }
    } requests_;
};

void StreamImpl::SockHandler::ready(What event) noexcept {
    if (isWrite(event)) {
        stream_->writeHelper();
    }

    // TODO: reads
}

StreamImpl::WriteRequest::WriteRequest(const char *buffer, size_t size,
        WriteCallback *cb) : callback_(cb), next_(nullptr) {
    buffer_.append(buffer, size);
}

StreamImpl::WriteRequest::~WriteRequest() { }

void StreamImpl::write(const char *buf, size_t size, WriteCallback *cb) {
    // TODO: try writing immediately, saving a loop iteration if the socket
    // can handle the entire write w/o blocking
    WriteRequest *req = new WriteRequest(buf, size, cb);
    requests_.append(req);
    base_->registerHandler(&handler_, ensureWrite(handler_.watched()));
}

StreamImpl::~StreamImpl() {
    WriteRequest *r;
    // Delete all outstanding write requests
    while ((r = requests_.consumeFront()) != nullptr) { }
}

void StreamImpl::writeHelper() {
    WriteRequest *req = requests_.head;
    std::vector<Extent> extents;
    bool blocked = false;
    bool failed = false;
    while (req) {
        extents.clear();
        // TODO: better limit
        req->buffer_.peek(std::numeric_limits<size_t>::max(), &extents);
        assert(!extents.empty());

        // TODO: writev
        size_t total_written = 0;
        for (auto& extent : extents) {
            ssize_t written = ::write(handler_.fd(), extent.data, extent.size);
            if (written > 0) {
                total_written += written;
            } else if (written == 0) {
                blocked = true;
                break;
            } else {
                failed = true;
                break;
            }
        }

        req->buffer_.drain(total_written);

        if (blocked || failed) {
            break;
        }

        if (req->callback_) {
            req->callback_->complete(this);
        }

        req = requests_.consumeFront();
    }

    if (failed && req->callback_) {
        // TODO: better errors
        req->callback_->error(std::runtime_error("Write failed"));
    }
}

Stream* wrapFd(EventBase *base, int fd) {
    return new StreamImpl(base, fd);
}

} // wte namespace
