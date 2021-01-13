#pragma once
#include <lkl.h>
#include <queue.h>

ns_lite_kernel_lib_begin

class thread_t;

typedef void (*thread_func)(void* arg);

class task_mgr
{
private:
    static uint32_t          s_states;
    static queue_t<thread_t> s_rdy_queue;
    static queue_t<thread_t> s_all_queue;
public:
    enum
    {
        TMS_INITIALIZED = 0x00000001,
        TMS_MAIN_THREAD = 0x00000002,
        TMC_BASE_PRIOR  = 30
    };
public:
    static void
    init();
    
    // 'scheduler' is an ISR for 'timer'
    static void
    scheduler(uint32_t no /*never used*/);

    static thread_t*
    current_thread();
    
    static bool
    begin_thread(
        thread_func func,
        void*       arg,
        const char* name,
        uint32_t    prior
    );

    static void
    block_current_thread();

    static void
    unblock_thread(thread_t* th);

protected:
    static void
    task_switch(
        thread_t* cur,
        thread_t* next
    );

    // a simple unique tid
    // should pray for OS will not create thread more than 0xFFFFFFFF
    // :-)
    static uint32_t
    make_tid() {
        static uint32_t s_tid = 1;
        return ++s_tid;
    }

    static void
    __inner_cur_thrd_as_main_thrd();

private:
    // creating an instance is disallowed
    task_mgr() {};    
};


ns_lite_kernel_lib_end
