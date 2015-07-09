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

#ifndef WTE_EXECUTORS_LOOP_EXECUTOR_H_
#define WTE_EXECUTORS_LOOP_EXECUTOR_H_

#include "wte/event_base.h"

namespace wte {

class LoopExecutor {
public:
    LoopExecutor();
    ~LoopExecutor();

    /** Enqueue `task` for execution. */
    template<typename Task>
    void execute(Task &&task);

    template<typename Task>
    void execute(Task const& task);

    /**
     * Process all tasks enqueued before this method was invoked.
     */
    void loop();
private:
    EventBase *event_base_;
};

template<>
void LoopExecutor::execute(std::function<void(void)> &&task);

template<typename Task>
void LoopExecutor::execute(Task &&task) {
    execute(std::function<void(void)>(
        std::bind([this](Task const& task) -> void {
            task();
        }, std::move(task))));
}

template<typename Task>
void LoopExecutor::execute(Task const& task) {
    execute([this, &task]() -> void {
            task();
        });
}

} // wte namespace

#endif // WTE_EXECUTORS_LOOP_EXECUTOR_H_
