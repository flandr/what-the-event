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

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/thread.h>

#if !defined(_WIN32)
#include <pthread.h>
#endif

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <mutex>

#include "event_handler_impl.h"
#include "libevent_event_handler.h"
#include "wte/event_base.h"
#include "wte/event_handler.h"

namespace wte {

class LibeventEventBase final : public EventBase {
public:
    LibeventEventBase();
    ~LibeventEventBase();

    void loop(LoopMode mode) override;
    void stop() override;
    void registerHandler(EventHandler*, What) override;
    void unregisterHandler(EventHandler*) override;
private:
    event_base *base_;
    std::atomic<bool> terminate_;
#if !defined(_WIN32)
    std::atomic<pthread_t> loopThread_;
#else
    std::atomic<HANDLE> loopThread_;
#endif

    struct {
        std::mutex mutex;
        std::condition_variable cv;
        bool finished = false;
    } await_;
};

LibeventEventBase::LibeventEventBase() : base_(event_base_new()),
        terminate_(false) {
    // TODO: lock-free event loop
#ifdef _WIN32
    evthread_use_windows_threads();
#else
    evthread_use_pthreads();
#endif
}

LibeventEventBase::~LibeventEventBase() {
    event_base_free(base_);
}

namespace {
void persistentTimerCb(evutil_socket_t, int16_t, void*) { }

int toFlags(What what) {
    switch (what) {
    case What::NONE:
        return 0;
    case What::READ:
        return EV_READ;
    case What::WRITE:
        return EV_WRITE;
    case What::READ_WRITE:
        return EV_READ | EV_WRITE;
    }
}

void libeventCallback(evutil_socket_t fd, int16_t flags, void *ctx) {
    reinterpret_cast<EventHandler*>(ctx)->ready(fromFlags(flags));
}

} // unnamed namespace

void LibeventEventBase::loop(LoopMode mode) {
    struct event persistent_timer;
    int rc = 0;

#if !defined(_WIN32)
    loopThread_.store(pthread_self(), std::memory_order_release);
#else
    loopThread_.store(GetCurrentThread(), std::memory_order_release);
#endif

    await_.finished = false;

    if (mode == LoopMode::FOREVER) {
        // Enqueue a persistent event for versions of libevent that
        // don't support EVLOOP_NO_EXIT_ON_EMPTY
        static struct timeval tv = { 3600, 0 };
        event_assign(&persistent_timer, base_, -1, 0, persistentTimerCb,
            nullptr);
        if (0 != event_add(&persistent_timer, &tv)) {
            throw std::runtime_error("Internal error starting loop");
        }
    }

    while (!terminate_.load(std::memory_order_acquire)) {

        rc = event_base_loop(base_, EVLOOP_ONCE);
        // event_base_loop can exit prematurely; for example, the Windows
        // select-based backend may terminate if the network interfaces
        // become unavailable (select will exit with WSAENETDOWN). We thus
        // ignore error return values.

        if (mode == LoopMode::ONCE) {
            break;
        }

        if (mode == LoopMode::UNTIL_EMPTY && rc == 1) {
            // rc == 1 means that libevent has no more registered entries
            break;
        }
    }

    if (mode == LoopMode::FOREVER) {
        if (-1 == event_del(&persistent_timer)) {
            assert(0);
        }
    }

    // Reset the termination flag on the way out
    terminate_.store(false, std::memory_order_release);
    loopThread_.store(0, std::memory_order_release);

    {
        // Notify waiters
        std::lock_guard<std::mutex> lock(await_.mutex);
        await_.finished = true;
        await_.cv.notify_all();
    }
}

void LibeventEventBase::stop() {
    terminate_.store(true, std::memory_order_release);
    event_base_loopexit(base_, nullptr);
    {
        std::unique_lock<std::mutex> lock(await_.mutex);
        await_.cv.wait(lock, [this]() { return await_.finished; });
    }
}

void LibeventEventBase::unregisterHandler(EventHandler *handler) {
    if (!handler->base()) {
        return;
    }

    assert(handler->base() == this);

    LibeventEventHandler *impl = reinterpret_cast<LibeventEventHandler*>(
        EventHandlerImpl::get(handler));

    if (!impl || !impl->registered()) {
        return;
    }

    event_del(&impl->event_);
    impl->registered_ = false;
}

void LibeventEventBase::registerHandler(EventHandler *handler, What what) {
    if (handler->base()) {
        assert(handler->base() == this);
        if (what == EventHandlerImpl::get(handler)->watched()) {
            // No change
            return;
        }
    }

    if (what == What::NONE) {
        unregisterHandler(handler);
        return;
    }

    LibeventEventHandler *impl = reinterpret_cast<LibeventEventHandler*>(
        EventHandlerImpl::get(handler));

    if (impl && impl->registered()) {
        // Need to peel off the existing event to update its flags
        event_del(&impl->event_);
    }

    if (!impl) {
        impl = new LibeventEventHandler(this);
        EventHandlerImpl::set(handler, impl);
    }

    event_assign(&impl->event_, base_, handler->fd(),
        toFlags(what) | EV_PERSIST, libeventCallback, handler);

    impl->registered_ = true;

    event_add(&impl->event_, /*timeout=*/ nullptr);
}

EventBase* mkEventBase() {
    return new LibeventEventBase();
}

} // wte namespace
