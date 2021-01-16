#include <inbb.h>
#include <debug.h>
#include <tskmgr.h>
#include <x86/asm.h>

ns_lite_kernel_lib_begin

void
inbb_t::putc(char c)
{
    ASSERT(!x86_asm::is_interrupt_on());
    while(full()) {
        _lock.acquire();
        // here is another similar solution
        // it locks the whole 'put_c' scope
        // they also protect the 'buffer'
        // in that case, there might be other 'waiters' in the waiting
        // list, but first waiter wakes up first.
        
        // this implementation only locks _producer/_consumer, buffer isn't
        // in the critical section. which means, the first 'producer' is
        // waiting.
        // then, there's a consumer 'getc', now, the buffer isn't either
        // full or empty.
        // before that 'consumer' issues signal to inform this first
        // producer.
        // 'task_switch' happens, then another 'producer' invokes 'put_c',
        // he won't get in the while() scope. so, that producer will
        // inoke 'putc' successfully. the 'buffer' became full again
        // 'task_switch' happens again, the 'consumer' continues to
        // 'signal' the first 'producer'.
        // when producer back to while loop, he release lock, then while() 
        // he will realize that 'buffer' is still full. then go to sleep
        // again.
        // this while() is so called 'Mesa Monitor'. it can be replaced
        // with 'if'. that's a 'Hoard Monitor'. 'Hoard Monitor' is easier
        // to explain, but think that buffer only has one slot.
        // another 'producer' already wrote a 'char', then this 'producer'
        // wakes up, then you got an 'overflow'.
        wait(true);
        _lock.release();
    }

    _buf[_head] = c;
    _head = next(_head);

    if(_consumer != nullptr) {
        signal(false);
    }
}

char
inbb_t::getc()
{
    ASSERT(!x86_asm::is_interrupt_on());
    while(empty()) {
        _lock.acquire();
        wait(false);
        _lock.release();
    }

    char c = _buf[_tail];
    // I think there is a bug, consider this
    // suppose buffer has one 'char', a consumer get there and
    // take a 'char', before _tail = next(_tail)
    // another consumer gets there (by task_switch, buffer not empty)
    // he takes one. then task_switch happens.
    // now, two consumers took same 'char' and they will increase _tail
    // twice.
    // no producer produced anything, suddenly, buffer is full. because
    // next(_head) == _tail
    //
    // the berkeley's version puts all stull in critical section which
    // prevented this from happenning.
    _tail = next(_tail);

    if(_producer != nullptr) {
        signal(true);
    }
    return c;
}

void
inbb_t::wait(bool producer)
{
    if(producer) {
        ASSERT(_producer == nullptr);
        _producer = task_mgr::current_thread();
    } else {
        ASSERT(_consumer == nullptr);
        _consumer = task_mgr::current_thread();
    }
    task_mgr::block_current_thread();
}

void
inbb_t::signal(bool producer) {
    thread_t* th = nullptr;
    if(producer) {
        th = _producer;
        _producer = nullptr;
    } else {
        th = _consumer;
        _consumer = nullptr;
    }

    ASSERT(th != nullptr);
    task_mgr::unblock_thread(th);
}

ns_lite_kernel_lib_end