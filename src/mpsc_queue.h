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

#ifndef SRC_MPSC_QUEUE_H_
#define SRC_MPSC_QUEUE_H_

#include <atomic>

#include "cache_aligned.h"
#include "optional.h"

namespace wte {

/**
 * A concurrent multi-producer, single-consumer, lock-free, FIFO queue.
 *
 * This is an extension of Dmitry Vyukov's MPSC non-intrusive queue
 * (bit.ly/vyukov-mpsc). The extension trades the cost of producer/consumer
 * containtion at the tail for added functionality of consumers being able to
 * infer that the consumer *may* have observed an empty queue. This enables a
 * use model where fewer "work available" notifications are sent to a
 * non-busy-waiting consumer when there are concurrent producers. TODO: profiling
 * this trade-off under representative loads would be welcome.
 *
 * The element type `T` must be default-constructible.
 */
template<typename T>
class ConcurrentMPSCQueue {
public:
    ConcurrentMPSCQueue()
        : head_(mkAligned<Node>()),
          tail_(head_.load()) { }

    ~ConcurrentMPSCQueue() {
        while (this->pop()) { }
        freeAligned(head_.load());
    }

    /** Pop the next element off the queue. */
    Optional<T> pop();

    /**
     * Enqueue an item.
     *
     * @return whether the queue was empty at insertion time.
     */
    bool push(T const& item);
    bool push(T && item);
private:
    struct Node {
        T data;
        std::atomic<Node*> next;
        Node() : next(nullptr) { }
        explicit Node(T && data) : data(std::move(data)), next(nullptr) { }
    };

    std::atomic<Node*> head_;
    std::atomic<Node*> tail_;
};

template<typename T>
bool ConcurrentMPSCQueue<T>::push(T const& item) {
    return push(T(item));
}

template<typename T>
bool ConcurrentMPSCQueue<T>::push(T && item) {
    Node *cur = mkAligned<Node>(std::move(item));

    // Swap the previous head with the new node. Producers serialize here.
    Node *prev = head_.exchange(cur, std::memory_order_acq_rel);

    // assert prev->next == nullptr

    // Check whether the queue appears to be empty here. The access to the
    // tail requires acquire semantics. Any producers that arrive after we've
    // grabbed head will not be visible to the consumer until we publish
    // our new value.
    bool emptyAtInit = prev == tail_.load(std::memory_order_acquire);

    // Publish to the consumer
    prev->next.store(cur, std::memory_order_release);

    if (emptyAtInit) {
        return true;
    } else {
        // The queue was empty when we arrived, but the consumer _may_
        // have observed an empty queue before we pubished our value. If
        // the current tail_ is equal to the observed head_ at entry
        // (local variable prev), the consumer may have observed this empty
        // queue. We need to return true in this case, even though it may
        // be a false positive.
        if (tail_.load(std::memory_order_acquire) == prev) {
            return true;
        }
    }

    return false;
}

template<typename T>
Optional<T> ConcurrentMPSCQueue<T>::pop() {
    // Only the consumer updates tail, so we can read it with relaxed ordering
    Node *tail = tail_.load(std::memory_order_relaxed);
    Node *next = tail->next;

    if (!next) {
        // Empty
        return Optional<T>();
    }

    // Publish to producers with release semantics. In the original algorithm
    // producers don't read this value, so relaxed semantics were acceptable.
    tail_.store(next, std::memory_order_release);
    freeAligned(tail);
    return Optional<T>(std::move(next->data));
}

} // wte namespace

#endif // SRC_MPSC_QUEUE_H_
