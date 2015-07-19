/*
 * Copyright © 2015 Nathan Rosenblum <flander@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <memory>

#include "event_base_test.h"
#include "wte/connection_listener.h"
#include "wte/stream.h"

namespace wte {

class EchoServer {
public:
    struct Connection;

    explicit EchoServer(EventBase *base) : base(base) {
        listener = mkConnectionListener(base, [this](int fd) -> void {
                Connection *conn = new Connection(wrapFd(this->base, fd));
                conn->stream->startRead(&conn->read_cb);
            },
            [this](std::exception const&) -> void {
                // Nothing
            });
        listener->bind(0);
        listener->listen(128);
        listener->startAccepting();
    }

    int16_t port() {
        return listener->port();
    }

    class WriteCallback final : public Stream::WriteCallback {
    public:
        explicit WriteCallback(Connection *conn) : conn(conn) { }
        void complete(wte::Stream *) override { }
        void error(std::runtime_error const&);

        Connection *conn;
    };

    class ReadCallback final : public Stream::ReadCallback {
    public:
        explicit ReadCallback(Connection *conn) : conn(conn) { }
        void available(wte::Buffer *buf) override;
        void eof() override;
        void error(std::runtime_error const&);

        Connection *conn;
    };

    struct Connection {
        explicit Connection(Stream *stream) : stream(stream), read_cb(this),
            write_cb(this) { }
        ~Connection() {
            stream->stopRead();
            stream->close();
            delete stream;
        }
        Stream *stream;
        ReadCallback read_cb;
        WriteCallback write_cb;
    };

    EventBase *base;
    ConnectionListener *listener;
};

void EchoServer::WriteCallback::error(std::runtime_error const&) {
    delete conn;
}

void EchoServer::ReadCallback::available(wte::Buffer *buf) {
    conn->stream->write(buf, &conn->write_cb);
}

void EchoServer::ReadCallback::eof() {
    delete conn;
}

void EchoServer::ReadCallback::error(std::runtime_error const&) {
    delete conn;
}

class StreamTest : public EventBaseTest {
public:
    class TestWriteCallback final : public Stream::WriteCallback {
    public:
        void complete(Stream *) override {
            completed = true;
        }
        void error(std::runtime_error const&) override {
            errored = true;
        }
        bool completed = false;
        bool errored = false;
    };

    class TestReadCallback final : public Stream::ReadCallback {
    public:
        void available(Buffer *buf) override {
            total_read += buf->size();
            buf->drain(buf->size());
        }

        void eof() override {
            hit_eof = true;
        }

        void error(std::runtime_error const&) override {
            errored = true;
        }

        size_t total_read = 0;
        bool hit_eof = false;
        bool errored = false;
    };

    class TestConnectCallback final : public Stream::ConnectCallback {
    public:
        void complete() override {
            completed = true;
        }

        void error(std::runtime_error const&) override {
            errored = true;
        }
        bool completed = false;
        bool errored = false;
    };
};

TEST_F(StreamTest, WritesRaiseCallbackOnCompletion) {
    TestWriteCallback cb1;
    TestWriteCallback cb2;

    std::unique_ptr<Stream> stream(wrapFd(base, fds[0]));

    char buf[64];
    memset(buf, 'A', sizeof(buf));
    stream->write(buf, sizeof(buf), &cb1);
    stream->write(buf, sizeof(buf), &cb2);

    // Using "UNTIL_EMPTY" to verify that write handlers are unregistered
    // upon completion of the write.
    base->loop(EventBase::LoopMode::UNTIL_EMPTY);

    ASSERT_TRUE(cb1.completed);
    ASSERT_TRUE(cb2.completed);

    char read_buf[128];
    int nread = xread(fds[1], read_buf, sizeof(read_buf));
    ASSERT_EQ(128, nread);
}

TEST_F(StreamTest, LargeWrites) {
    TestWriteCallback wcb;
    TestReadCallback rcb;

    std::unique_ptr<Stream> wstream(wrapFd(base, fds[0]));
    std::unique_ptr<Stream> rstream(wrapFd(base, fds[1]));

    const int kLarge = 1 << 20; // "large"
    char *wbuf = new char[kLarge];
    memset(wbuf, 'A', kLarge);
    wstream->write(wbuf, kLarge, &wcb);
    rstream->startRead(&rcb);

    while (rcb.total_read < kLarge) {
        // Have to drive this repeatedly because the pipe has only
        // so much capacity
        base->loop(EventBase::LoopMode::ONCE);
    }

    delete [] wbuf;
}

TEST_F(StreamTest, WriteErrorsRaiseCallback) {
    // TODO: this appears to be racy on Windows, insofar as the write
    // may not detect an error on the closed connection. I'm not sure
    // whether that's expected behavior or not and need to check.
#if !defined(_WIN32)
    TestWriteCallback cb;
    std::unique_ptr<Stream> stream(wrapFd(base, fds[0]));

    char buf[64];
    memset(buf, 'A', sizeof(buf));

    // Close the other end of the pipe
    closepipe<1>();

    stream->write(buf, sizeof(buf), &cb);
    base->loop(EventBase::LoopMode::ONCE);
    base->loop(EventBase::LoopMode::ONCE);
    ASSERT_FALSE(cb.completed);
    ASSERT_TRUE(cb.errored);
#endif
}

TEST_F(StreamTest, ReadableDataRaisesCallback) {
    TestWriteCallback wcb;
    TestReadCallback rcb;

    std::unique_ptr<Stream> wstream(wrapFd(base, fds[0]));
    std::unique_ptr<Stream> rstream(wrapFd(base, fds[1]));

    wstream->write("ping", 4, &wcb);
    rstream->startRead(&rcb);

    base->loop(EventBase::LoopMode::ONCE);
    base->loop(EventBase::LoopMode::ONCE);

    ASSERT_TRUE(wcb.completed);
    ASSERT_FALSE(rcb.errored);
    ASSERT_EQ(4, rcb.total_read);
}

TEST_F(StreamTest, CloseRaisesEofCallback) {
    TestReadCallback rcb;
    std::unique_ptr<Stream> rstream(wrapFd(base, fds[1]));
    rstream->startRead(&rcb);
    rstream->close();
    ASSERT_TRUE(rcb.hit_eof);

    // prevent double-close
    fds[1] = -1;
}

TEST_F(StreamTest, TestConnect) {
    EchoServer echo(base);

    TestConnectCallback ccb;
    auto* stream = Stream::create(base);
    stream->connect("127.0.0.1", echo.port(), &ccb);

    // Nothing happens until we drive the loop
    ASSERT_FALSE(ccb.completed);
    ASSERT_FALSE(ccb.errored);

    base->loop(EventBase::LoopMode::ONCE);

    EXPECT_TRUE(ccb.completed);
    EXPECT_FALSE(ccb.errored);
}

} // wte namespace
