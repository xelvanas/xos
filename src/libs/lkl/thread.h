#pragma once
#include <lkl.h>
#include <string.h>
#include <queue.h>

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



//
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

class pcb_t
{
private:
    uint32_t*      _k_stack;
    uint32_t       _state;
    uint32_t       _prior;
    char           _name[16];
    uint32_t       _magic;
    qnode_t<pcb_t> _node;
public:
    enum 
    {
        TS_RUNNING,
        TS_READY,
        TS_WAITING,
        TS_BLOCKED,
        TS_HANGING
    };

    void set_kstack(uint32_t* stack) {
        _k_stack = stack;
    }

    uint32_t* get_kstack() const {
        return _k_stack;
    }

    void set_state(uint32_t state) {
        _state = state;
    }

    uint32_t get_state() const {
        return _state;
    }

    void set_prior(uint32_t prior) {
        _prior = prior;
    }

    uint32_t get_prior() const {
        return _prior;
    }

    void set_name(const char* name) {
        strcpy_s(_name, 16, name);
    }

    const char* get_name() const {
        return _name;
    }

    void set_magic(uint32_t magic) {
        _magic = magic;
    }

    uint32_t get_magic() const {
        return _magic;
    }

    qnode_t<pcb_t>& get_qnode() {
        return _node;
    }

    const qnode_t<pcb_t>& get_qnode() const {
        return _node;
    }
};

class thread_t
{
private:

public:
    


};

class task_mgr
{
private:
    static uint32_t       s_task_mgr_states;
    static queue_t<pcb_t> s_ready_queue;
    static queue_t<pcb_t> s_all_queue;

public:
    enum
    {
        TMS_INITIALIZED = 0x00000001,
        TMS_MAIN_THREAD = 0x00000002,
    };
public:

    static void   init();
    
    // this is an ISR for 'timer'
    static void   scheduler(uint32_t no);
    
    static pcb_t* current_pcb();
private:
    task_mgr() {};
private:
    static void __inner_cur_thrd_as_main_thrd();
};

ns_lite_kernel_lib_end