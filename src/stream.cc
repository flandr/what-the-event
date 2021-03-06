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

#include <errno.h>

#if !defined(_WIN32)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#else
#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <cassert>
#include <limits>

#include <event2/util.h>

#include "buffer-internal.h"
#include "wte/buffer.h"
#include "wte/event_base.h"
#include "wte/event_handler.h"
#include "wte/porting.h"
#include "xplat-io.h"

namespace wte {

inline bool isConnectRetryable(int e) {
#if !defined(_WIN32)
    return e == EINPROGRESS;
#else
    return e == WSAEWOULDBLOCK || e == WSAEINPROGRESS || e == WSAEINTR;
#endif
}

inline bool isReadRetryable(int e) {
#if !defined(_WIN32)
    return e == EAGAIN || e == EWOULDBLOCK;
#else
    return e == WSAEWOULDBLOCK || e == WSAEINTR;
#endif
}

class StreamImpl final : public Stream {
public:
    // TODO: temporary fd-based constructor for testing
    StreamImpl(std::shared_ptr<EventBase> base, int fd) : handler_(this, fd),
        base_(base), requests_({nullptr, nullptr}), readCallback_(nullptr),
        connectCallback_(nullptr) { }

    explicit StreamImpl(std::shared_ptr<EventBase> base) : handler_(this, -1),
        base_(base), requests_({nullptr, nullptr}), readCallback_(nullptr),
        connectCallback_(nullptr) { }

    ~StreamImpl();

    void write(const char *buf, size_t size, WriteCallback *cb) override;
    void write(Buffer *buf, WriteCallback *cb) override;
    void startRead(ReadCallback *cb) override;
    void stopRead() override;
    void close() override;
    void connect(std::string const& ip_addr, int16_t port, ConnectCallback *cb)
        override;
private:
    void writeHelper();
    void readHelper();
    void connectHelper();

    class SockHandler final : public EventHandler {
    public:
        SockHandler(StreamImpl *stream, int fd)
            : EventHandler(fd), stream_(stream) { }
        void ready(What what) NOEXCEPT override;
    private:
        StreamImpl *stream_;
    };

    // State about a write request (buffer, callback)
    class WriteRequest {
    public:
        WriteRequest(const char *buffer, size_t size, WriteCallback *cb);
        WriteRequest(Buffer *buf, WriteCallback *cb);
        ~WriteRequest();
        BufferImpl buffer_;
        WriteCallback *callback_;
        WriteRequest *next_;
    };

    SockHandler handler_;
    std::shared_ptr<EventBase> base_;
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
    ReadCallback *readCallback_;
    ConnectCallback *connectCallback_;
    BufferImpl readBuffer_;
};

void StreamImpl::SockHandler::ready(What event) NOEXCEPT {
    if (isWrite(event)) {
        if (stream_->connectCallback_) {
            stream_->connectHelper();
        }
        stream_->writeHelper();
    }

   if (isRead(event)) {
       stream_->readHelper();
   }
}

StreamImpl::WriteRequest::WriteRequest(const char *buffer, size_t size,
        WriteCallback *cb) : callback_(cb), next_(nullptr) {
    buffer_.append(buffer, size);
}

StreamImpl::WriteRequest::WriteRequest(Buffer *buffer, WriteCallback *cb)
        : callback_(cb), next_(nullptr) {
    buffer_.append(buffer);
}

StreamImpl::WriteRequest::~WriteRequest() { }

void StreamImpl::startRead(Stream::ReadCallback *cb) {
    if (readCallback_ == cb) {
        return;
    }
    readCallback_ = cb;
    base_->registerHandler(&handler_, ensureRead(handler_.watched()));
}

void StreamImpl::stopRead() {
    if (!readCallback_) {
        return;
    }

    readCallback_ = nullptr;

    What events = handler_.watched();
    if (!isRead(handler_.watched())) {
        return;
    } else if (isWrite(handler_.watched())) {
        assert(events == What::READ_WRITE);
        base_->registerHandler(&handler_, What::WRITE);
    }
}

void StreamImpl::write(const char *buf, size_t size, WriteCallback *cb) {
    // TODO: try writing immediately, saving a loop iteration if the socket
    // can handle the entire write w/o blocking
    WriteRequest *req = new WriteRequest(buf, size, cb);
    requests_.append(req);
    base_->registerHandler(&handler_, ensureWrite(handler_.watched()));
}

void StreamImpl::write(Buffer *buf, WriteCallback *cb) {
    WriteRequest *req = new WriteRequest(buf, cb);
    requests_.append(req);
    base_->registerHandler(&handler_, ensureWrite(handler_.watched()));
}

void StreamImpl::close() {
    if (readCallback_) {
        readCallback_->eof();
    }
    if (connectCallback_) {
        connectCallback_->error(std::runtime_error("Closed before connect"));
    }
    if (handler_.registered()) {
        handler_.unregister();
    }

    if (handler_.fd() != -1) {
        xclose(handler_.fd());
    }
}

void StreamImpl::connect(std::string const& ip, int16_t port,
        ConnectCallback *cb) {
    assert(handler_.fd() == -1);
    assert(connectCallback_ == nullptr);

    const char *error = nullptr;
    int fd = -1;

    for (;;) {
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (-1 == fd) {
            error = "Failed to allocate socket";
            break;
        }

        int rc = evutil_make_socket_nonblocking(fd);
        if (-1 == rc) {
            error = "Failed to set socket non-blocking";
            break;
        }

        struct sockaddr_in saddr;
        saddr.sin_family = AF_INET;
        rc = inet_pton(AF_INET, ip.c_str(), &saddr.sin_addr);
        if (1 != rc) {
            error = "Failed to convert address";
            break;
        }

        saddr.sin_port = htons(port);
        socklen_t len = sizeof(saddr);
        rc = ::connect(fd, reinterpret_cast<struct sockaddr*>(&saddr), len);
        if (rc == -1) {
            if (isConnectRetryable(evutil_socket_geterror(fd))) {
                // Expected; queue up the callback
                handler_.setFd(fd);
                connectCallback_ = cb;
                base_->registerHandler(&handler_, ensureWrite(
                    handler_.watched()));
                return;
            } else {
                error = "Connect failed";
                break;
            }
        }

        // Connect succeeded immediately
        handler_.setFd(fd);
        cb->complete();

        return;
    }

    if (fd != -1) {
        xclose(fd);
    }

    cb->error(std::runtime_error(error));
}

StreamImpl::~StreamImpl() {
    handler_.unregister();
    WriteRequest *r;
    // Delete all outstanding write requests
    while ((r = requests_.consumeFront()) != nullptr) { }
}

void StreamImpl::readHelper() {
    char buf[4096];

    // TODO: consider not reading indefinitely
    for (;;) {
        int nread = xread(handler_.fd(), buf, sizeof(buf));
        if (nread < 0) {
            if (isReadRetryable(evutil_socket_geterror(handler_.fd()))) {
                break;
            }
            // TODO: better errors
            if (readCallback_) {
                readCallback_->error(std::runtime_error("Read failed"));
            }
            break;
        } else if (nread == 0) {
            if (readCallback_) {
                readCallback_->eof();
            }
            break;
        }

        // TODO: a reserve + get readable iovec + readv will be more efficient
        // than a read + append.
        readBuffer_.append(buf, nread);

        if (readCallback_) {
            readCallback_->available(&readBuffer_);
        }
        if (nread < sizeof(buf)) {
            break;
        }
    }
}

void StreamImpl::connectHelper() {
    assert(connectCallback_);

    const char *error = nullptr;

    for (;;) {
        // Check connection status
        int err = 0;
        socklen_t len = sizeof(error);
#if defined(_WIN32)
        int rc = getsockopt(handler_.fd(), SOL_SOCKET, SO_ERROR,
            (char *) &err, &len);
#else
        int rc = getsockopt(handler_.fd(), SOL_SOCKET, SO_ERROR, &err, &len);
#endif
        if (-1 == rc) {
            error = "Failed to query connection status";
            break;
        }

        if (error != 0) {
            error = "Connection failed";
            break;
        }

        // Good to go
        auto *cb = connectCallback_;
        connectCallback_ = nullptr;
        cb->complete();
        return;
    }

    auto *cb = connectCallback_;
    connectCallback_ = nullptr;
    cb->error(std::runtime_error(error));
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
            int written = xwrite(handler_.fd(), extent.data, extent.size);
            if (written >= 0) {
                total_written += written;
                if (written < extent.size) {
                    blocked = true;
                }
            } else {
                failed = true;
                break;
            }
        }

        req->buffer_.drain(total_written);

        if (blocked || failed) {
            break;
        }

        WriteCallback *cb = req->callback_;
        // Must not touch req after this call
        WriteRequest *next = requests_.consumeFront();
        if (!next) {
            // Uninstall the write handler before invoking the callback
            // for this final request; callbacks that know that they are
            // last invocation may legitimately do destructive things like
            // freeing this stream.
            base_->registerHandler(&handler_, removeWrite(handler_.watched()));
        }

        if (cb) {
            cb->complete(this);
        }

        req = next;
    }

    if (failed && req->callback_) {
        // TODO: better errors
        req->callback_->error(std::runtime_error("Write failed"));
    }
}

void Stream::Deleter::operator()(Stream *stream) {
    delete stream;
}

std::unique_ptr<Stream, Stream::Deleter> wrapFd(std::shared_ptr<EventBase> base,
        int fd) {
    return std::unique_ptr<Stream, Stream::Deleter>(
         new StreamImpl(base, fd), Stream::Deleter());
}

std::unique_ptr<Stream, Stream::Deleter> Stream::create(
        std::shared_ptr<EventBase> base) {
    return std::unique_ptr<Stream, Stream::Deleter>(
         new StreamImpl(base), Stream::Deleter());
}

} // wte namespace
