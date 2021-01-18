#pragma once
#include <lkl.h>
#include <semaphore.h>

ns_lite_kernel_lib_begin

class thread_t;

class lock_t
{
private:
    thread_t*   _holder   = nullptr;
    uint32_t    _lock_cnt = 0;
    semaphore_t _sema;
    
public:
    lock_t() 
    : _sema(1) {
    }

    void
    acquire();

    void
    release();
};

class lock_guard
{
    lock_t& _lock;
public:
    
    lock_guard(lock_t& lock) : _lock(lock) {
        _lock.acquire();
    }

    ~lock_guard() {
        _lock.release();
    }

    // non-assignable
    lock_guard(lock_guard const&) = delete;
    lock_guard& operator=(lock_guard const&) = delete;
private:
};

ns_lite_kernel_lib_end