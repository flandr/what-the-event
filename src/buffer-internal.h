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

#ifndef SRC_BUFFER_INTERNAL_H_
#define SRC_BUFFER_INTERNAL_H_

#include <vector>

#include "wte/buffer.h"

namespace wte {

class BufferImpl {
public:
    BufferImpl();
    ~BufferImpl();

    void append(const char *buf, size_t size);
    void append(BufferImpl *o);
    void prepend(const char *buf, size_t size);
    void prepend(BufferImpl *o);

    // Copy data out, consuming in the process
    void read(char *buf, size_t size, size_t *nread);

    // Copy data out, without consuming
    void peek(char *buf, size_t size, size_t *nread) const;

    // Peek at data without consuming, using extents
    void peek(size_t size, std::vector<Extent> *extents) const;

    // Drain data
    void drain(size_t count);

    bool empty() const;
    size_t size() const { return size_; }
    void reserve(size_t capacity);
    void reserve(size_t capacity, std::vector<Extent> *extents);

    struct InternalExtent {
        Extent extent;
        size_t read_offset;
        size_t write_offset;

        struct InternalExtent *prev;
        struct InternalExtent *next;

        explicit InternalExtent(size_t size) : extent({size, new char[size]}),
            read_offset(0), write_offset(0), prev(nullptr), next(nullptr) { }

        InternalExtent() : extent({0, nullptr}), read_offset(0),
            write_offset(0), prev(nullptr), next(nullptr) { }

        ~InternalExtent() {
            delete [] extent.data;
        }

        size_t appendable() const {
            return extent.size - write_offset;
        }

        size_t prependable() const {
            return read_offset;
        }

        size_t readable() const {
            return extent.size - read_offset;
        }

        bool empty() const { return appendable() == 0; }

        size_t append(const char *buf, size_t size);
        size_t prepend(const char *buf, size_t size);
        size_t copyout(char *buf, size_t size);
        size_t consume(size_t size);
    };

    static BufferImpl* get(Buffer *buf);
private:
    bool list_empty() const;
    void read(char *buf, size_t size, size_t *nread, bool consume);

    // Does not free memory; take care
    void reset() {
        head_.prev = &head_;
        head_.next = &head_;
        size_ = 0;
    }

    InternalExtent head_;
    size_t size_;
};

} // wte namespace

#endif // SRC_BUFFER_INTERNAL_H_
