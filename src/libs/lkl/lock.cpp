#include <lock.h>
#include <tskmgr.h>

ns_lite_kernel_lib_begin

void
lock_t::acquire()
{
    if(_holder != task_mgr::current_thread()) {
        _sema.down();
        _holder   = task_mgr::current_thread();
        _lock_cnt = 1;

    } else {
        ++_lock_cnt;
    }
}

void
lock_t::release()
{
    if(_lock_cnt > 1) {
        --_lock_cnt;
    } else {
        _holder   = nullptr;
        _lock_cnt = 0;
        _sema.up();
    }
}

ns_lite_kernel_lib_end