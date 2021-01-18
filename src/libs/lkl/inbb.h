#pragma once
#include <lkl.h>
#include <lock.h>
#include <scode.h>


ns_lite_kernel_lib_begin

class thread_t;

// interrupt bounded-buffer
class inbb_t
{
    enum
    {
        BUF_SIZE = 128
    };
private:
    lock_t      _lock;
    scode_t     _buf[BUF_SIZE];
    int32_t     _head           = 0;
    int32_t     _tail           = 0;
    thread_t*   _producer       = nullptr;
    thread_t*   _consumer       = nullptr;
public:
    inline int32_t
    next(int32_t pos) const {
        return (pos + 1) % BUF_SIZE;
    }

    inline bool
    full() const {
        return next(_head) == _tail;
    };

    inline bool
    empty() const {
        return _head == _tail;
    }

    void
    putc(scode_t c);

    scode_t
    getc();

    void
    wait(bool who);

    void
    signal(bool producer);
};

ns_lite_kernel_lib_end