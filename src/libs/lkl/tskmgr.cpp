#include <tskmgr.h>
#include <thread.h>
#include <memory.h>
#include <x86/asm.h>
#include <intmgr.h>

extern "C" void __task_switch(
    uint32_t* __th1_stack,
    uint32_t* __th2_stack);

ns_lite_kernel_lib_begin

// ------------------------- Low Mem Address ---------------------------
// the order is from bottom (high mem address) to top (low mem addrss)
// [U]:  marks 'user space only'
// [Sx]: marks which step, for example: [S3], [S5]
struct intr_stack
{
    // step 8: ready to call c++ interrupt handler
    // step 7: pushes interrupt vector number
    uint32_t _vct;
    
    // step 6: handler saved environment registers 
    uint32_t _edi;
    uint32_t _esi;
    uint32_t _ebp;
    uint32_t _esp_dummy; // popad won't restore this
    uint32_t _ebx;
    uint32_t _edx;
    uint32_t _ecx;
    uint32_t _eax;
    uint32_t _gs;
    uint32_t _fs;
    uint32_t _es;
    uint32_t _ds;
    // here is where 'interrupt handlers' take over. programmers decide
    // whether to save those register or not (better to do) and in which
    // order.


    // CPU has done its work
    // those works mentioned below are done by CPU automatically, we
    // don't have to worry about it. after our 'interrupt handlers' done
    // its job, 'iret' will take us back to [S0].

    // step 5: pushes 'error code' if appropriate
    uint32_t _err;
    // NOTE: no 'Stack Switching' if [S0] is a kernel program, CPU 
    // pushes eflags, cs, eip directly.
    // step 4: pushes 'eip', where CPU back to [S0] 
    uint32_t _eip;
    // step 4: pushes 'cs'
    uint32_t _cs;
    // step 4: pushes 'eflags'
    uint32_t _eflags;
    // step 4:[U] pushes 'esp' into esp saved at [S2]
    uint32_t _esp;
    // step 4:[U] pushes 'ss'  into esp saved at [S2]
    uint32_t _ss;
    // [S3] which is called 'Stack Switching', only occurs when system
    ///     is running different PRIVILEGE program (CPL != 0). 
    // step 3: [U] load ss0, esp0 from TSS into 'ss', 'esp'
    // step 2: [U] CPU saves ss, esp, eflags, cs and eip temporarily 
    // step 1: interrupt occurs
    // step 0: user/kernel space program running
};
// ------------------------- High Mem Address --------------------------

struct thread_stack
{
    uint32_t _ebp;
    uint32_t _ebx;
    uint32_t _edi;
    uint32_t _esi;

    // eip: kernerl_function to execute our thread_function
    uint32_t _eip;
    /*
     * caller of 'kernerl_function'
     * pseudocode:
     *    void caller_of_kernel_func()
     *    {
     *         // call kernel function
     *         // 'call' instruction will do following:
     *         //  push  thread_arg
     *         //  push  thread_func
     *         //  push  eip ; for 'ret' to go back there [DUMMPY EIP]
     *         //  jmp   ker_func
     *         //  the 'push'+'jmp' is a 'call' instruction
     *         ker_func(thread_func, thread_arg);
     *    }
     * it's not real code. but we will fake those procedures. so there
     * is no real 'caller_of_kernel_func'. all the reason _eip_dummy 
     * exists is just for padding. of course we can 'ret again to invoke
     * 'thread_func', but it's harder to pass an 'argument' in that way.
     */
    uint32_t _eip_dummy; 
    // thread function
    // why don't we just simplely 'ret' to this 'thread_func'?
    // because before our thread_func executes, there're jobs to do.
    // for example: we should 'enable interrupt' again to prevent
    // 'thread_func' be came exclusive (no other thread can enter).
    uint32_t _fun;
    // thread argument
    uint32_t _arg;
};

// -----------------------------------------------------------------------
// Task Manager

uint32_t          task_mgr::s_states = 0;
queue_t<thread_t> task_mgr::s_rdy_queue;
queue_t<thread_t> task_mgr::s_all_queue;

// why do we have this rather than invoking 'thread function'
// directly? before 'thread function' runs, kernel has work to do.
// that work is not a part of 'user thread', it cannot be put into 
// 'thread function'. neither can be put  into 'scheduler', because
// scheduler don't whether it's a new thread or not.
void
prep_ent_thread(
    thread_func func,
    void* arg)
{
    x86_asm::turn_interrupt_on();
    func(arg);
}

void
task_mgr::init()
{
    // already initialized
    if(bit_test(s_states, TMS_INITIALIZED))
        return;

    // order is important, do not reg 'scheduler' before
    // main thread initialized.
    __inner_cur_thrd_as_main_thrd();

    // register scheduler as ISR for 
    // interrupt<x86_asm>::reg(
    //     pic8259a::DEV_TIMER,
    //     task_mgr::scheduler);
}

thread_t* task_mgr::current_thread() {
    uint32_t esp = x86_asm::get_esp();
    return (thread_t*)(esp & thread_t::TH_MASK);
}

void 
task_mgr::scheduler(uint32_t no) 
{

    auto cur = current_thread();

    // ASSERT(s_all_queue.find(&cur->get_alq_node()));

    if(s_rdy_queue.empty()) {
        // no need to turn interrupt on, iret will restore eflags later
        return;
    }

    if(cur->is_running()) {
        cur->state(thread_t::TS_READY);
        s_rdy_queue.push_back(&cur->node());
    }

    auto node = s_rdy_queue.pop_front();
    (*node)->state(thread_t::TS_RUNNING);
    task_switch(cur, node->get());
}

bool
task_mgr::begin_thread(
    thread_func func,
    void*       arg,
    const char* name,
    uint32_t    prior)
{
    ASSERT(func != nullptr);
    if(func == nullptr) {
        return false;
    }

    auto th    = (thread_t*)mem_mgr::alloc(mem_mgr::PT_KERNEL, 1);
    
    if(th == nullptr) {
        return false;
    }

    auto addr  = (uint32_t)th + PAGE_SIZE;
         addr -= sizeof(intr_stack);
         addr -= sizeof(thread_stack);

    // prepare task switching data
    auto ts    = (thread_stack*)addr;
    ts->_fun   = (uint32_t)func;
    ts->_arg   = (uint32_t)arg;
    ts->_eip   = (uint32_t)prep_ent_thread;

    // initialize thread
    th->tid(make_tid());
    if(name != nullptr && strlen(name) < thread_t::TH_NAME_LEN) {
        th->name(name);
    }
    
    th->base_prior(TMC_BASE_PRIOR);
    th->reset_prior();
    th->kstack((uint32_t*)addr);
    th->state(thread_t::TS_READY);
    
    th->node().reset(th);
    th->anode().reset(th);
    th->cast_magic();
    s_rdy_queue.push_back(th->node_ptr());
    s_all_queue.push_back(th->anode_ptr());
    return true;
}

void
task_mgr::block_current_thread()
{
    ASSERT(x86_asm::is_interrupt_on() == false);
    current_thread()->state(thread_t::TS_BLOCKED);
    scheduler(0);
}

void
task_mgr::unblock_thread(thread_t* th)
{
    ASSERT(
        th != nullptr &&
        th->is_magic_dashed() == false &&
        th->is_blocked());

    intr_guard guard(false);
    
    th->state(thread_t::TS_READY);
    s_rdy_queue.push_back(th->node_ptr());
}


// bool
// task_mgr::create_process(uint32_t func)
// {
//     if(func == 0)
//         return false;

//     // one page for PCB
//     auto pcb = mem_mgr::alloc(mem_mgr::PT_KERNEL, 1);
//     ASSERT(pcb != nullptr);

//     // one page for 'page table'
//     auto cr3 = mem_mgr::alloc(mem_mgr::PT_KERNEL, 1);
//     ASSERT(cr3 != nullptr);

//     auto cr3_phys = mem_mgr::v2p((uint32_t)cr3);

    
//     return false;
// }
bool
task_mgr::create_process(uint32_t func)
{
    // int arr[3] = {1,2,3};
    // for(auto& x : arr) {

    // }
    // auto [a, b ,c] = arr;
    // auto  x = a;
    return false;
}

// -----------------------------------------------------------------------
// Private Members of Task Manager

void 
task_mgr::task_switch(
    thread_t* cur,
    thread_t* next)
{
    __task_switch(
        (uint32_t*)cur->kstack_addr(),
        (uint32_t*)next->kstack_addr()
    );
}

void
task_mgr::__inner_cur_thrd_as_main_thrd() {
    // main thread initialized
    if(bit_test(s_states, TMS_MAIN_THREAD))
        return;

    auto th = current_thread();
    // it's a dangeous operation,  'esp' can be somewhere 
    // between 'pcb + sizeof(pcb_t)'.  :)
    ASSERT((uint32_t)th + sizeof(thread_t) < x86_asm::get_esp());

    memset((void*)th, 0, sizeof(thread_t));
    // initialize main thread
    th->name("main");
    // pcb and kernel stack share same page
    // don't worry about this stack address
    // 'task_switch' function will replace it with a correct value
    th->kstack((uint32_t*)((uint32_t)th + 0x1000));

    th->state(thread_t::TS_RUNNING);
    th->cast_magic();
    th->node().reset(th);
    th->anode().reset(th);
    s_all_queue.push_back(th->anode_ptr());
}

// 
// -----------------------------------------------------------------------

ns_lite_kernel_lib_end