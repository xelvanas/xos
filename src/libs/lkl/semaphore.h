#pragma once
#include <lkl.h>
#include <queue.h>

ns_lite_kernel_lib_begin

class thread_t;

class semaphore_t
{
private:
    uint32_t            _value;
    queue_t<thread_t>   _wait_queue;
public:
    // avoid 0 to prevent deadlock
    semaphore_t(uint32_t val) 
        : _value(val != 0 ? val : 1) {
    }

    /* 
     * the Dutch Computer Scientist Dijkstra introduced a pair of
     * synchronization primitives called 'P' and 'V'.
     * 'P' = dutch word 'Probeer' (try)
     * 'V' = dutch word 'Verhoog' (increment)
     * verhogen
     */

    // 'P':down()
    // waits until semaphore._value > 1
    void
    down();

    // 'V':up()
    // increase semaphore._value
    // wake up another thread in wait queue
    void
    up();
};

ns_lite_kernel_lib_end