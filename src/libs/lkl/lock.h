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

class auto_lock
{
    lock_t& _lock;
public:
    
    auto_lock(lock_t& lock) : _lock(lock) {
        _lock.acquire();
    }

    ~auto_lock() {
        _lock.release();
    }

    // non-assignable
    auto_lock(auto_lock const&) = delete;
    auto_lock& operator=(auto_lock const&) = delete;
private:
};

ns_lite_kernel_lib_end