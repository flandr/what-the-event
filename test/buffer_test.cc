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

#include <string>

#include <gtest/gtest.h>

#include "wte/buffer.h"

namespace wte {

std::string contents(Buffer const& buffer) {
    size_t size = buffer.size();
    char *buf = new char[size];
    size_t nread = 0;
    buffer.peek(buf, size, &nread);
    EXPECT_EQ(size, nread);
    std::string ret(buf, nread);
    delete [] buf;
    return ret;
}

std::unique_ptr<Buffer, Buffer::Deleter> mkBuffer(std::string const& str) {
    auto ret = Buffer::create();
    ret->append(str.c_str(), str.size());
    return ret;
}

std::unique_ptr<Buffer, Buffer::Deleter> mkBuffer() {
    return Buffer::create();
}

TEST(BufferTest, TestStartsEmpty) {
    auto buf = mkBuffer();
    ASSERT_TRUE(buf->empty());
}

TEST(BufferTest, TestStartsWithZeroSize) {
    auto buf = mkBuffer();
    ASSERT_EQ(0U, buf->size());
}

TEST(BufferTest, TestAppend) {
    const char kBuf[] = "foo";
    auto buf = mkBuffer();
    buf->append(kBuf, strlen(kBuf));
    ASSERT_EQ(strlen(kBuf), buf->size());
    ASSERT_FALSE(buf->empty());
    ASSERT_EQ(kBuf, contents(*buf));
}

TEST(BufferTest, TestAppendBuffer) {
    auto buf = mkBuffer();
    auto buf2 = mkBuffer("foo");
    buf->append(buf2.get());
    ASSERT_FALSE(buf->empty());
    ASSERT_TRUE(buf2->empty());
    ASSERT_EQ("foo", contents(*buf));
}

TEST(BufferTest, TestAppendString) {
    const std::string kBuf{"foo"};
    auto buf = mkBuffer();
    buf->append(kBuf);
    ASSERT_EQ(kBuf.size(), buf->size());
    ASSERT_FALSE(buf->empty());
    ASSERT_EQ(kBuf, contents(*buf));
}

TEST(BufferTest, TestPrepend) {
    const std::string kBuf1 { "bar" };
    const std::string kBuf2 { "foo" };
    auto buf = mkBuffer(kBuf1);
    buf->prepend(kBuf2.c_str(), kBuf2.size());
    ASSERT_EQ(kBuf1.size() + kBuf2.size(), buf->size());
    ASSERT_EQ("foobar", contents(*buf));
}

TEST(BufferTest, TestPrependBuffer) {
    auto buf = mkBuffer("bar");
    auto buf2 = mkBuffer("foo");
    buf->prepend(buf2.get());
    ASSERT_EQ("foobar", contents(*buf));
}

TEST(BufferTest, TestPrependString) {
    const std::string kBuf1 { "bar" };
    const std::string kBuf2 { "foo" };
    auto buf = mkBuffer(kBuf1);
    buf->prepend(kBuf2);
    ASSERT_EQ(kBuf1.size() + kBuf2.size(), buf->size());
    ASSERT_EQ("foobar", contents(*buf));
}

TEST(BufferTest, TestPeek) {
    auto buf = mkBuffer("foobar");
    char out[3];
    size_t nread = 0;
    buf->peek(out, 3, &nread);
    ASSERT_EQ(3U, nread);
    ASSERT_EQ("foo", std::string(out, 3));

    // Doing it again yields the same results
    memset(out, 0, sizeof(out));
    nread = 0;
    buf->peek(out, 3, &nread);
    ASSERT_EQ(3U, nread);
    ASSERT_EQ("foo", std::string(out, 3));

    // Peeking after a drain works as expected
    nread = 0;
    buf->drain(3);
    buf->peek(out, 3, &nread);
    ASSERT_EQ(3U, nread);
    ASSERT_EQ("bar", std::string(out, 3));
}

TEST(BufferTest, TestPeekSingleExtent) {
    auto buf = mkBuffer("foobar");
    buf->append("raboof", 6);
    std::vector<Extent> extents;
    buf->peek(3U, &extents);
    ASSERT_EQ(1U, extents.size());
    ASSERT_EQ("foo", std::string(extents[0].data, extents[0].size));

    extents.clear();
    buf->peek(6U, &extents);
    ASSERT_EQ(1U, extents.size());
}

TEST(BufferTest, TestPeekMultipleExtents) {
    auto buf = mkBuffer("foobar");
    buf->append("raboof", 6);
    std::vector<Extent> extents;
    buf->peek(9U, &extents);
    ASSERT_EQ(2U, extents.size());
    ASSERT_EQ("foobar", std::string(extents[0].data, extents[0].size));
    ASSERT_EQ("rab", std::string(extents[1].data, extents[1].size));
}

TEST(BufferTest, TestDrain) {
    auto buf = mkBuffer("foobar");
    ASSERT_EQ(6U, buf->size());
    buf->drain(0);
    ASSERT_EQ(6U, buf->size());
    buf->drain(1);
    ASSERT_EQ(5U, buf->size());
    buf->drain(1000);
    ASSERT_EQ(0U, buf->size());
}

TEST(BufferTest, TestRead) {
    auto buf = mkBuffer("foobar");
    char out[3];
    size_t nread = 0;
    buf->read(out, 3, &nread);
    ASSERT_EQ(3U, nread);
    ASSERT_EQ("foo", std::string(out, 3));
    // Data were consumed
    ASSERT_EQ(3U, buf->size());

    // Long reads are clamped
    buf->read(out, 100, &nread);
    ASSERT_EQ(3U, nread);
    ASSERT_EQ("bar", std::string(out, 3));

    // Reads on empty are empty
    buf->read(out, 100, &nread);
    ASSERT_EQ(0U, nread);
}

TEST(BufferTest, TestReserve) {
    auto buf = mkBuffer("foo");
    buf->reserve(10);
    ASSERT_EQ(3U, buf->size());
    buf->append("0123456789", 10);
    ASSERT_EQ(13U, buf->size());
    ASSERT_EQ("foo0123456789", contents(*buf));
}

TEST(BufferTest, TestReserveWithExtents) {
    auto buf = mkBuffer("");
    std::vector<Extent> extents;
    buf->reserve(10, &extents);
    ASSERT_EQ(1U, extents.size());
    ASSERT_NE(nullptr, extents[0].data);
    ASSERT_EQ(10U, extents[0].size);

    extents.clear();
    buf->reserve(15, &extents);
    ASSERT_EQ(2U, extents.size());
    ASSERT_EQ(10U, extents[0].size);
    ASSERT_EQ(5U, extents[1].size);
}

} // wte namespace
