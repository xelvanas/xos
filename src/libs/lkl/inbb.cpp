#include <inbb.h>
#include <debug.h>
#include <tskmgr.h>
#include <x86/asm.h>
#include <inbb.h>

ns_lite_kernel_lib_begin

void
inbb_t::putc(scode_t sc)
{
    //dbg_mhl("getc consumer:", (uint32_t)task_mgr::current_thread());
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

    _buf[_head] = sc;
    _head = next(_head);

    if(_consumer != nullptr) {
        signal(false);
    }
}

scode_t
inbb_t::getc()
{
    //dbg_mhl("getc consumer:", (uint32_t)task_mgr::current_thread());
    


    ASSERT(!x86_asm::is_interrupt_on());
    while(empty()) {
        _lock.acquire();
        //dbg_mhl("getc consumer1:", (uint32_t)_consumer);
        wait(false);
        _lock.release();
    }

    scode_t sc = _buf[_tail];
    // I think there is a bug.
    // suppose that buffer has one 'char', a consumer gets there and
    // takes a 'char' before '_tail = next(_tail)'.
    // another consumer gets there (by 'task_switch', buffer isn't empty.)
    // the second consumer takes one. then 'task_switch' switched to new
    // thread, now, two consumers took same 'char' and they will increase
    //  _tail twice.
    // 
    // no producer produced anything, suddenly, buffer is full. because
    // next(_head) == _tail
    //
    // the berkeley's version puts all stull in critical section which
    // prevented this from happenning.
    _tail = next(_tail);

    if(_producer != nullptr) {
        signal(true);
    }
    return sc;
}

void
inbb_t::wait(bool who)
{
    if(who) { //producer
        ASSERT(_producer == nullptr);
        _producer = task_mgr::current_thread();
    } else {
        if( _consumer != nullptr) {
            // dbg_mhl("consumer:", (uint32_t)_consumer);
            // while(1);
        }
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
        //dbg_mhl("signal producer:", (uint32_t)_producer);
        _producer = nullptr;
    } else {
        th = _consumer;
        //dbg_mhl("signal consumer:", (uint32_t)_consumer);
        _consumer = nullptr;
    }

    ASSERT(th != nullptr);
    task_mgr::unblock_thread(th);
}

ns_lite_kernel_lib_end