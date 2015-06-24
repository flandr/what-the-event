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

#include <string.h>
#include <unistd.h>

#if !defined(_WIN32)
#include <pthread.h>
#endif

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <mutex>

#include <event2/event.h>
#include <event2/event_struct.h>

#include "event_handler_impl.h"
#include "libevent_event_handler.h"
#include "mpsc_queue.h"
#include "timeout_impl.h"
#include "wte/event_base.h"
#include "wte/event_handler.h"
#include "wte/timeout.h"

namespace wte {

namespace { struct NotifyInit; }

class LibeventEventBase final : public EventBase {
public:
    LibeventEventBase();
    ~LibeventEventBase();

    void loop(LoopMode mode) override;
    void stop() override;
    void registerHandler(EventHandler*, What) override;
    void unregisterHandler(EventHandler*) override;
    bool runOnEventLoop(std::function<void(void)> const& op) override;
    bool runOnEventLoopAndWait(std::function<void(void)> const& op) override;
    void registerTimeout(Timeout *, struct timeval *duration) override;
    void unregisterTimeout(Timeout *) override;

    class NotifyHandler final : public EventHandler {
    public:
        NotifyHandler(LibeventEventBase *base, int fd)
                : EventHandler(fd), base_(base) { }
        void ready(What event) noexcept {
            switch (event) {
            case What::READ:
                base_->receiveNotifications();
                break;
            default:
                // XXX log?
                break;
            }
        }
    private:
        LibeventEventBase *base_;
    };

    struct Notify {
        enum class Type { PIPE, SOCKETPAIR, EVENTFD };
        Type type;
        ConcurrentMPSCQueue<std::function<void(void)>> queue;
        // Listen on 0, write on 1 (except eventfd, which is both on 1)
        int fds[2];
        NotifyHandler handler;
        explicit Notify(LibeventEventBase *base, NotifyInit const&);
    };
private:
    void receiveNotifications();
    void runOpsInQueue();
    bool consumeNotification();
    bool signalNotifyQueue();

    // In the loop thread or loop is not running
    bool inLoopThread();

    // In the loop thread
    bool inRunningLoopThread();

    void registerHandlerInternal(EventHandler*, What, bool internal_event);

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

    struct Notify notify_;
};

namespace {

class LibeventTimeout final : public TimeoutImpl {
public:
    explicit LibeventTimeout(LibeventEventBase *base) : base_(base) {
        memset(&event_, 0, sizeof(event_));
    }
    EventBase* base() override { return base_; }

    LibeventEventBase *base_ = nullptr;
    struct event event_;
    bool registered_ = false;
};

// Sigh.
struct NotifyInit {
    int fds[2];
    LibeventEventBase::Notify::Type type;
};

NotifyInit initNotify() {
    NotifyInit ret;

    ret.fds[0] = ret.fds[1] = -1;

    int rc = 0;
#if defined(HAVE_EVENTFD)
    ret.type = LibeventEventBase::Notify::Type::EVENTFD;
    ret.fds[0] = eventfd(0, EFD_CLOEXEC);
    if (ret.fds[0] < 0) {
        rc = -1;
    }
#elif !defined(_WIN32)
    ret.type = LibeventEventBase::Notify::Type::PIPE;
    rc = pipe(ret.fds);
#else
    ret.type = LibeventEventBase::Notify::Type::SOCKETPAIR;
    rc = evutil_socketpair(AF_INET, SOCK_STREAM, 0, ret.fds);
#endif
    if (rc) {
        throw std::runtime_error("Error configuring notification descriptors");
    }
    return ret;
}
} // unnamed namespace

LibeventEventBase::LibeventEventBase() : base_(event_base_new()),
        terminate_(false), loopThread_(0), notify_(this, initNotify()) {
    registerHandlerInternal(&notify_.handler, What::READ, /*internal=*/ true);
}

LibeventEventBase::Notify::Notify(LibeventEventBase *base,
            NotifyInit const& init)
        : handler(base, init.fds[0]) {
    fds[0] = init.fds[0];
    fds[1] = init.fds[1];
    type = init.type;
}

LibeventEventBase::~LibeventEventBase() {
    notify_.handler.unregister();

    event_base_free(base_);

    if (notify_.fds[0] > 0) {
        close(notify_.fds[0]);
    }
    if (notify_.fds[1] > 0) {
        close(notify_.fds[1]);
    }
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

void libeventTimeout(evutil_socket_t fd, int16_t flags, void *ctx) {
    reinterpret_cast<Timeout*>(ctx)->expired();
}

} // unnamed namespace

bool LibeventEventBase::inLoopThread() {
    // Relaxed order is sufficient; if we're running in the loop-driving thread,
    // whatever value was written is already visible to us (by definition). If
    // we're not the loop thread, either we assigned the value to 0 as we
    // exited the loop or by definition the value is != us.
    auto cur = loopThread_.load(std::memory_order_relaxed);
#if !defined(_WIN32)
    return cur == 0 || pthread_equal(cur, pthread_self());
#else
    return cur == 0 || cur == GetCurrentThread();
#endif
}

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
        // Always run ops in the notification queue
        runOpsInQueue();

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

bool LibeventEventBase::runOnEventLoop(std::function<void(void)> const& op) {
    if (inLoopThread()) {
        op();
        return true;
    }

    bool shouldKick = notify_.queue.push(op);

    if (shouldKick) {
        return signalNotifyQueue();
    }

    return true;
}

bool LibeventEventBase::runOnEventLoopAndWait(
        std::function<void(void)> const& op) {
    if (inLoopThread()) {
        op();
        return true;
    }

    bool done = false;
    std::mutex mutex;
    std::condition_variable cv;

    bool scheduled = runOnEventLoop([&]() -> void {
            op();
            {
                std::lock_guard<std::mutex> lock(mutex);
                done = true;
                cv.notify_one();
            }
        });
    if (!scheduled) {
        return false;
    }

    // Await completion
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [&done]() -> bool { return done; });

    return true;
}

bool LibeventEventBase::consumeNotification() {
    ssize_t rbytes;
    uint64_t val = 0;
    uint64_t val8 = 0;

    switch (notify_.type) {
    case Notify::Type::EVENTFD:
        rbytes = ::read(notify_.fds[0], &val, sizeof(val));
        break;
    default:
        rbytes = ::read(notify_.fds[0], &val8, sizeof(val8));
    }
    if (rbytes <= 0) {
        return false;
    }
    return true;
}

bool LibeventEventBase::signalNotifyQueue() {
    // Make 64 bits available for eventfd-based notification queues
    const uint8_t buf[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };
    int ret = 0;
    switch (notify_.type) {
    case Notify::Type::PIPE:
    case Notify::Type::SOCKETPAIR:
#if defined(_WIN32)
        ret = send(notify_.fds[1], buf, 1, 0);
#else
        ret = write(notify_.fds[1], buf, 1);
#endif
        break;
    case Notify::Type::EVENTFD:
#if defined(HAVE_EVENTFD)
        ret = write(notify_.fds[0], sizeof(buf));
#else
        assert(0 && "eventfd not supported");
        ret = -1;
#endif
    }

    if (ret <= 0) {
        return false;
    }
    return true;
}

void LibeventEventBase::receiveNotifications() {
    // Consume one event from the notification queue. There may be more,
    // but the event is level-triggered and we'll wake up again and
    // consume them.
    bool consumed = consumeNotification();
    if (!consumed) {
        // XXX log?
        return;
    }

    runOpsInQueue();
}

void LibeventEventBase::runOpsInQueue() {
    // Execute all available messages
    for (;;) {
        auto op = notify_.queue.pop();
        if (!op) {
            // Empty
            break;
        }
        op.value()();
    }
}

void LibeventEventBase::stop() {
    runOnEventLoop([this]() -> void {
        terminate_.store(true, std::memory_order_release);
        event_base_loopexit(base_, nullptr);
    });

    {
        std::unique_lock<std::mutex> lock(await_.mutex);
        await_.cv.wait(lock, [this]() { return await_.finished; });
    }
}

void LibeventEventBase::unregisterHandler(EventHandler *handler) {
    assert(inLoopThread());

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

void LibeventEventBase::registerTimeout(Timeout *timeout,
        struct timeval *duration) {
    assert(inLoopThread());

    LibeventTimeout *ltime = nullptr;
    {
        auto* impl = TimeoutImpl::get(timeout);
        if (!impl) {
            ltime = new LibeventTimeout(this);
        } else {
            ltime = reinterpret_cast<LibeventTimeout*>(impl);
            TimeoutImpl::set(timeout, ltime);
        }
    }

    if (ltime->registered_) {
        event_del(&ltime->event_);
    }

    // TODO: error checking & throw
    event_assign(&ltime->event_, base_, -1, 0, libeventTimeout, timeout);
    event_add(&ltime->event_, duration);

    ltime->registered_ = true;
}

void LibeventEventBase::unregisterTimeout(Timeout *timeout) {
    assert(inLoopThread());

    auto* impl = TimeoutImpl::get(timeout);
    if (!impl) {
        return;
    }

    if (!impl->base()) {
        return;
    }

    assert(impl->base() == this);

    LibeventTimeout* limpl = reinterpret_cast<LibeventTimeout*>(impl);

    if (!limpl->registered_) {
        return;
    }
    event_del(&limpl->event_);
    limpl->registered_ = false;
}

void LibeventEventBase::registerHandler(EventHandler *handler, What what) {
    assert(inLoopThread());
    registerHandlerInternal(handler, what, /*internal event=*/ false);
}

void LibeventEventBase::registerHandlerInternal(EventHandler *handler,
        What what, bool internal_event) {
    if (handler->base() && !internal_event) {
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
    if (internal_event) {
        // The inter-base notification channel has a registered event on the
        // base. We need to mark this internal so that it doesn't count against
        // the set of active events when looping, or the loop will never
        // terminate. This is brittle wrt forwards compatibility :/. An
        // alternative would be to maintain a count of registered internal
        // events and break out of the loop when we get down to zero (since
        // we always drive the loop manually).
        impl->event_.ev_flags |= EVLIST_INTERNAL;
    }

    impl->registered_ = true;

    event_add(&impl->event_, /*timeout=*/ nullptr);
}

EventBase* mkEventBase() {
    return new LibeventEventBase();
}

} // wte namespace
