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
#include "wte/stream.h"

namespace wte {

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
        void available(const char *, size_t size) override {
            total_read += size;
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
    ssize_t nread = read(fds[1], read_buf, sizeof(read_buf));
    ASSERT_EQ(128, nread);
}

TEST_F(StreamTest, WriteErrorsRaiseCallback) {
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
    // TODO: close & assert eof
}

} // wte namespace
