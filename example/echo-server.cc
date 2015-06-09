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

#include "wte/connection_listener.h"
#include "wte/event_base.h"
#include "wte/stream.h"

#include <stdio.h>

struct Connection;

class EchoWriteCallback final : public ::wte::Stream::WriteCallback {
public:
    explicit EchoWriteCallback(Connection *conn) : conn_(conn) { }
    void complete(wte::Stream *) { }
    void error(std::runtime_error const& e) override;
private:
    Connection *conn_;
};

class EchoReadCallback final : public ::wte::Stream::ReadCallback {
public:
    EchoReadCallback(Connection *conn, EchoWriteCallback *wcb)
        : conn_(conn), wcb_(wcb) { }
    void available(const char *buf, size_t size) override;
    void eof() override;
    void error(std::runtime_error const& e) override;
private:
    Connection *conn_;
    EchoWriteCallback *wcb_;
};

struct Connection {
    Connection(wte::EventBase *base, int fd)
        : stream(wte::wrapFd(base, fd)),
          write_cb(this),
          read_cb(this, &write_cb) { }
    ~Connection() {
        stream->stopRead();
        stream->close();
        delete stream;
    }
    wte::Stream *stream;
    EchoWriteCallback write_cb;
    EchoReadCallback read_cb;
};


void EchoWriteCallback::error(std::runtime_error const& e) {
    fprintf(stderr, "While writing: %s\n", e.what());
    delete conn_;
}

void EchoReadCallback::eof() {
    delete conn_;
}

void EchoReadCallback::available(const char *buf, size_t size) {
    conn_->stream->write(buf, size, wcb_);
}

void EchoReadCallback::error(std::runtime_error const& e) {
    fprintf(stderr, "While reading: %s\n", e.what());
    delete conn_;
}

static void acceptCb(wte::EventBase *base, int fd) {
    Connection *conn = new Connection(base, fd);
    conn->stream->startRead(&conn->read_cb);
}

static void errorCb(std::exception const& e) {
    fprintf(stderr, "While listening: %s\n", e.what());
}

int main(int argc, char **argv) {
    auto* base = wte::mkEventBase();
    auto* listener = wte::mkConnectionListener(base,
        std::bind(acceptCb, base, std::placeholders::_1), errorCb);

    listener->bind(0);
    listener->listen(128);
    listener->startAccepting();

    printf("Ready to talk back on %d\n", listener->port());
    base->loop(wte::EventBase::LoopMode::FOREVER);

    delete listener;
    delete base;

    return 0;
}
