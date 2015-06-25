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

#ifndef WTE_BUFFER_H_
#define WTE_BUFFER_H_

#include <sys/types.h>

#include <vector> // TODO: erg, not a stable interface...

namespace wte {

class BufferImpl;

/** A contiguous extent of data. */
struct Extent {
    /** Size of the buffer in `data`. */
    size_t size;
    char *data;
};

/**
 * Buffer for aggregating data.
 *
 * A Buffer encapsulates one or more (possibly discontiguous) ranges of data.
 * It supports efficient append and prepend methods, and can be used to pass
 * data to and from event `Stream`s without copying.
 *
 * Methods that allocate memory can throw std::bad_alloc.
 */
class Buffer {
public:
    Buffer();
    ~Buffer();
    Buffer(Buffer&&);
    Buffer(Buffer const&) = delete;
    Buffer& operator=(Buffer const&) = delete;

    /**
     * Appends data to the buffer.
     *
     * @param buf the input buffer
     * @param size the input buffer size
     */
    void append(const char *buf, size_t size);

    /**
     * Appends all the data from another buffer to this buffer.
     *
     * The originating buffer is drained as a result of this operation
     * (i.e., buffer.empty() == true if this method returns OK).
     *
     * @param buf the input buffer
     */
    void append(Buffer *buffer);

    /**
     * Prepends data to the buffer.
     *
     * @param buf the input buffer
     * @param size the input buffer size
     */
    void prepend(const char *buf, size_t size);

    /**
     * Prepends all the data from another buffer to this buffer.
     *
     * The originating buffer is drained as a result of this operation
     * (i.e., buffer.empty() == true if this method returns OK).
     *
     * @param buf the input buffer
     */
    void prepend(Buffer *buffer);

    /**
     * Copy data out of the buffer, consuming it.
     *
     * This method reads at most `size` bytes from the buffer. If fewer
     * bytes are available, `*nread` will be less than `size` on return.
     *
     * @param buf the allocated output buffer
     * @param size number of bytes to read
     * @param nread number of bytes read
     */
    void read(char *buf, size_t size, size_t *nread);

    /**
     * Like `read`, but does not consume data in the buffer.
     *
     * Repeated invocations of this method read the same data until `drain`
     * or `read` are invoked.
     *
     * @param buf the allocated output buffer
     * @param size number of bytes to read
     * @param nread number of bytes read
     */
    void peek(char *buf, size_t size, size_t *nread) const;

    /**
     * Peek at up to `size` bytes, possibly discongiguous.
     *
     * @param size number of bytes to read
     * @param extents vector of 0 or more `Extents` containing pointers to data
     */
    void peek(size_t size, std::vector<Extent> *extents) const;

    /**
     * Consume up to `size` bytes of the buffer without reading.
     *
     * It is safe to invoke this method with any value >= 0, regardless of
     * the amount of data in the buffer.
     *
     * @param size maximum bytes to consumer
     */
    void drain(size_t size);

    /** @return whether the buffer is empty. */
    bool empty() const;

    /** @return the total amount of data in the buffer. */
    size_t size() const;

    /**
     * Reserve at least `size` bytes of space in the buffer.
     *
     * @param size the space to reserve
     */
    void reserve(size_t size);

    /**
     * Reserve at least `size` bytes of space in the buffer, returning 1 or
     * more writable extents comprising the reserved region(s).
     *
     * @param size the space to reserve
     * @param extents the reserved extents
     */
    void reserve(size_t size, std::vector<Extent> *extents);
private:
    BufferImpl *pImpl_;
    friend class BufferImpl;
};

} // wte namespace

#endif // WTE_BUFFER_H_
