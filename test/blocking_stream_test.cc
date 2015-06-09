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
#include "wte/blocking_stream.h"

namespace wte {

class BlockingStreamTest : public EventBaseTest {
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

    std::unique_ptr<Stream> mkStream(int fd) {
        return std::unique_ptr<Stream>(wrapFd(base, fd));
    }

    bool received(BlockingStream &reader, const char *buf, size_t size) {
        char *readbuf = new char[size];
        int64_t nread = reader.read(readbuf, size);
        if (nread != size) {
            delete [] readbuf;
            return false;
        }
        bool equal = (0 == memcmp(buf, readbuf, size));
        delete [] readbuf;
        return equal;
    }
};

TEST_F(BlockingStreamTest, Write) {
    BlockingStream writer(fds[0], /*close=*/ false);
    BlockingStream reader(fds[1], /*close=*/ false);

    char buf[64];
    memset(buf, 'A', sizeof(buf));

    writer.write(buf, sizeof(buf));
    ASSERT_TRUE(received(reader, buf, sizeof(buf)));
}

} // wte namespace
