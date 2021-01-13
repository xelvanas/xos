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

ns_lite_kernel_lib_end