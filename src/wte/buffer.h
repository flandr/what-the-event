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

#include <memory>
#include <string>
#include <vector> // TODO: erg, not a stable interface...

#include "wte/porting.h"

namespace wte {

/** A contiguous extent of data. */
struct WTE_SYM Extent {
    /** Size of the buffer in `data`. */
    size_t size;
    char *data;
};

class BufferImpl;

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
    virtual ~Buffer();
    Buffer(Buffer const&) = delete;
    Buffer& operator=(Buffer const&) = delete;

    /**
     * Appends data to the buffer.
     *
     * @param buf the input buffer
     * @param size the input buffer size
     */
    virtual void append(const char *buf, size_t size) = 0;

    /** Appends data to the buffer. */
    virtual void append(std::string const& buffer) = 0;

    /**
     * Appends all the data from another buffer to this buffer.
     *
     * The originating buffer is drained as a result of this operation
     * (i.e., buffer.empty() == true if this method returns OK).
     *
     * @param buf the input buffer
     */
    virtual void append(Buffer *buffer) = 0;

    /**
     * Prepends data to the buffer.
     *
     * @param buf the input buffer
     * @param size the input buffer size
     */
    virtual void prepend(const char *buf, size_t size) = 0;

    /** Prepends data to the buffer. */
    virtual void prepend(std::string const& buffer) = 0;

    /**
     * Prepends all the data from another buffer to this buffer.
     *
     * The originating buffer is drained as a result of this operation
     * (i.e., buffer.empty() == true if this method returns OK).
     *
     * @param buf the input buffer
     */
    virtual void prepend(Buffer *buffer) = 0;

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
    virtual void read(char *buf, size_t size, size_t *nread) = 0;

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
    virtual void peek(char *buf, size_t size, size_t *nread) const = 0;

    /**
     * Peek at up to `size` bytes, possibly discongiguous.
     *
     * @param size number of bytes to read
     * @param extents vector of 0 or more `Extents` containing pointers to data
     */
    virtual void peek(size_t size, std::vector<Extent> *extents) const = 0;

    /**
     * Consume up to `size` bytes of the buffer without reading.
     *
     * It is safe to invoke this method with any value >= 0, regardless of
     * the amount of data in the buffer.
     *
     * @param size maximum bytes to consumer
     */
    virtual void drain(size_t size) = 0;

    /** @return whether the buffer is empty. */
    virtual bool empty() const = 0;

    /** @return the total amount of data in the buffer. */
    virtual size_t size() const = 0;

    /**
     * Reserve at least `size` bytes of space in the buffer.
     *
     * @param size the space to reserve
     */
    virtual void reserve(size_t size) = 0;

    /**
     * Reserve at least `size` bytes of space in the buffer, returning 1 or
     * more writable extents comprising the reserved region(s).
     *
     * @param size the space to reserve
     * @param extents the reserved extents
     */
    virtual void reserve(size_t size, std::vector<Extent> *extents) = 0;

    struct WTE_SYM Deleter {
        void operator()(Buffer *buf);
    };

    static std::unique_ptr<Buffer, Deleter> create() {
        return std::unique_ptr<Buffer, Deleter>(mkBuffer());
    }
private:
    Buffer() { }
    friend class BufferImpl;

    WTE_SYM static Buffer* mkBuffer();
    WTE_SYM static void release(Buffer *);
};

} // wte namespace

#endif // WTE_BUFFER_H_
