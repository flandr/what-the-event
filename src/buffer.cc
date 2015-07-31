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

#include <string.h>

#include <algorithm>
#include <limits>
#include <utility>

#include "buffer-internal.h"
#include "wte/buffer.h"

namespace wte {

size_t BufferImpl::InternalExtent::append(const char *buf, size_t size) {
    char *data = extent.data;
    size_t space = write_offset - extent.size;
    if (space == 0) {
        return 0;
    }
    size_t nwrite = std::min(space, size);
    memcpy(data + write_offset, buf, nwrite);
    write_offset += nwrite;

    // assert write_offset <= extent.size

    return nwrite;
}

size_t BufferImpl::InternalExtent::prepend(const char *buf, size_t size) {
    char *data = extent.data;
    size_t space = read_offset;
    if (space == 0) {
        return 0;
    }
    size_t nwrite = std::min(space, size);
    memcpy(data, buf, nwrite);

    // assert read_offset <= nwrite

    read_offset -= nwrite;
    return nwrite;
}

size_t BufferImpl::InternalExtent::copyout(char *buf, size_t size) {
    size_t ret = std::min(size, extent.size - read_offset);
    memcpy(buf, extent.data + read_offset, ret);
    return ret;
}

size_t BufferImpl::InternalExtent::consume(size_t size) {
    // assert read_offset + size <= extent.size
    read_offset += size;
    return extent.size - read_offset;
}

BufferImpl::BufferImpl() : size_(0) {
    head_.prev = &head_;
    head_.next = &head_;
}

BufferImpl::~BufferImpl() {
    drain(std::numeric_limits<size_t>::max());
}

void BufferImpl::drain(size_t count) {
    size_t remain = count;
    while (remain > 0 && !list_empty()) {
        auto cur = head_.next;
        size_t consume = std::min(remain, cur->readable());
        size_t node_remain = cur->consume(consume);
        if (node_remain == 0) {
            cur->prev->next = cur->next;
            cur->next->prev = cur->prev;
            delete cur;
        }
        remain -= consume;
    }
    size_ -= (count - remain);
}

bool BufferImpl::empty() const {
    return size_ == 0;
}

inline void listAppend(BufferImpl::InternalExtent *head,
        BufferImpl::InternalExtent *head2, BufferImpl::InternalExtent *tail) {
    head2->prev = head->prev;
    head2->prev->next = head2;
    tail->next = head;
    tail->next->prev = tail;
}

inline void listAppend(BufferImpl::InternalExtent *head,
        BufferImpl::InternalExtent *node) {
    listAppend(head, node, node);
}

inline void listPrepend(BufferImpl::InternalExtent *head,
        BufferImpl::InternalExtent *head2, BufferImpl::InternalExtent *tail) {
    tail->next = head->next;
    tail->next->prev = tail;
    head2->prev = head;
    head2->prev->next = head2;
}

inline void listPrepend(BufferImpl::InternalExtent *head,
        BufferImpl::InternalExtent *node) {
    listPrepend(head, node, node);
}

bool BufferImpl::list_empty() const {
    return head_.prev == &head_; // also head_.next == &head_
}

void BufferImpl::append(const char *buf, size_t size) {
    InternalExtent *cur = nullptr;
    if (!list_empty()) {
        cur = head_.prev;
    }

    size_t remain = size;
    do {
        if (!cur || cur->appendable() == 0) {
            cur = new InternalExtent(remain);
            listAppend(&head_, cur);
        }
        remain -= cur->append(buf, remain);
    } while (remain > size);

    size_ += size;
}

void BufferImpl::append(std::string const& buf) {
    append(buf.c_str(), buf.size());
}

void BufferImpl::append(Buffer *orig) {
    BufferImpl *o = static_cast<BufferImpl*>(orig);

    if (o->list_empty()) {
        return;
    }

    listAppend(&head_, o->head_.next, o->head_.prev);
    size_ += o->size_;
    o->reset();
}

void BufferImpl::prepend(const char *buf, size_t size) {
    InternalExtent *cur = nullptr;
    if (!list_empty()) {
        cur = head_.next;
    }

    size_t remain = size;
    do {
        if (!cur || cur->prependable() == 0) {
            cur = new InternalExtent(remain);
            remain -= cur->append(buf, remain);

            // assert remain == 0

            listPrepend(&head_, cur);
        } else {
            remain -= cur->prepend(buf, remain);
        }
    } while (remain > 0);

    size_ += size;
}

void BufferImpl::prepend(std::string const& buf) {
    prepend(buf.c_str(), buf.size());
}

void BufferImpl::prepend(Buffer *orig) {
    BufferImpl *o = static_cast<BufferImpl*>(orig);

    if (o->list_empty()) {
        return;
    }

    listPrepend(&head_, o->head_.next, o->head_.prev);
    size_ += o->size_;
    o->reset();
}

void BufferImpl::reserve(size_t size) {
    size_t required = size;
    if (!list_empty()) {
        required -= head_.prev->appendable();
    }
    if (required > 0) {
        InternalExtent *cur = new InternalExtent(required);
        listAppend(&head_, cur);
    }
}

void BufferImpl::reserve(size_t size, std::vector<Extent> *extents) {
    extents->reserve(2);
    size_t required = size;
    if (!list_empty()) {
        InternalExtent *p = head_.prev;
        size_t avail = p->appendable();
        if (avail > 0) {
            extents->emplace_back(Extent{avail,
                p->extent.data + p->write_offset});
            required -= avail;
        }
    }
    if (required > 0) {
        InternalExtent *cur = new InternalExtent(required);
        listAppend(&head_, cur);
        extents->emplace_back(Extent{required, cur->extent.data});
    }
}

void BufferImpl::read(char *buf, size_t size, size_t *nread) {
    return read(buf, size, nread, /*consume=*/ true);
}

void BufferImpl::read(char *buf, size_t size, size_t *nread, bool consume) {
    InternalExtent *cur = head_.next;
    size_t total = 0;
    while (cur != &head_ && total < size) {
        size_t tmpread = cur->copyout(buf, size);
        total += tmpread;
        buf += tmpread;
        if (!consume) {
            cur = cur->next;
            continue;
        }

        size_ -= tmpread;

        size_t remain = cur->consume(tmpread);
        if (remain == 0) {
            cur->prev->next = cur->next;
            cur->next->prev = cur->prev;
            auto next = cur->next;
            delete cur;
            cur = next;
        } else {
            // assert total == size
            break;
        }
    }

    *nread = total;
}

void BufferImpl::peek(char *buf, size_t size, size_t *nread) const {
    return const_cast<BufferImpl*>(this)->read(buf, size, nread,
        /*consume=*/ false);
}

// Peek at data without consuming, using extents
void BufferImpl::peek(size_t size, std::vector<Extent> *extents) const {
    InternalExtent *cur = head_.next;
    size_t total = 0;
    while (cur != &head_ && total < size) {
        size_t avail = cur->readable();
        if (avail > 0) {
            size_t grant = std::min(avail, size - total);
            extents->push_back(
                Extent {grant, cur->extent.data + cur->read_offset});
        }
        total += avail;
        cur = cur->next;
    }
}

Buffer::~Buffer() { }

Buffer* Buffer::mkBuffer() {
    return new BufferImpl();
}

void Buffer::release(Buffer *buf) {
    delete buf;
}

void Buffer::Deleter::operator()(Buffer *buf) {
    delete buf;
}

} // wte namespace
