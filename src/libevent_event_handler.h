
#ifndef SRC_LIBEVENT_EVENT_HANDLER_H_
#define SRC_LIBEVENT_EVENT_HANDLER_H_

#include <event2/event_struct.h>

#include "event_handler_impl.h"

namespace wte {

class LibeventEventBase;

class LibeventEventHandler final : public EventHandlerImpl {
public:
    explicit LibeventEventHandler(EventBase *base)
        : base_(base), registered_(false) { }
    What watched() override;
    EventBase* base() override { return base_; }
    bool registered() override;
private:
    EventBase *base_;
    bool registered_;
    struct event event_;

    friend class LibeventEventBase;
};

What fromFlags(int16_t flags);

} // wte namespace

#endif // SRC_LIBEVENT_EVENT_HANDLER_H_
