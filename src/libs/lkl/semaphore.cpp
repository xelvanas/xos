#include <semaphore.h>
#include <thread.h>
#include <tskmgr.h>
#include <intmgr.h>

ns_lite_kernel_lib_begin

void
semaphore_t::down()
{
    intr_guard guard(false);
    while(_value == 0) {
        
        auto th = task_mgr::current_thread();
        ASSERT(th->is_running() && th->node().alone());
        _wait_queue.push_back(th->node_ptr());
        task_mgr::block_current_thread();
    }
    --_value;
}

void
semaphore_t::up()
{
    intr_guard guard(false);
    thread_t* th = _wait_queue.empty() ?
                   nullptr :
                   _wait_queue.pop_front()->get();


    if(th != nullptr) {
        task_mgr::unblock_thread(th);
    }
    ++_value;
    ASSERT(_value > 0);
}

ns_lite_kernel_lib_end