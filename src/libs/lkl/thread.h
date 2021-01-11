#pragma once
#include <lkl.h>
#include <string.h>
#include <queue.h>

ns_lite_kernel_lib_begin

struct intr_stack
{
    uint32_t _vct;
    uint32_t _edi;
    uint32_t _esi;
    uint32_t _ebp;
    uint32_t _esp_dummy; // popad will ignore esp
    uint32_t _ebx;
    uint32_t _edx;
    uint32_t _ecx;
    uint32_t _eax;
    uint32_t _gs;
    uint32_t _fs;
    uint32_t _es;
    uint32_t _ds;

    // low privilege to high privilege 
    uint32_t _error;
    uint32_t _eip;
    uint32_t _eflags;
    uint32_t _esp;
    uint32_t _ss;
};

struct thread_stack
{
    uint32_t _ebp;
    uint32_t _ebx;
    uint32_t _edi;
    uint32_t _esi;
    uint32_t _eip;
    uint32_t _ret;
    uint32_t _fun;
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
/*
 * Task Switching:
 * suppose we have a running thread A, it's esp is 0x01000.
 * now, timer interrupt occurs, and no privilege changed.
 * 
 * cpu will look into IDT and use 'interrupt vector' to locate the 
 * 'interrupt handler'
 * before we enter the 'handler', we assume that
 *                                         0x1000                   high
 * ┌─────────────────────────────────────────┬────────┬────────┬────────┐
 * │                                         │ Param1 │ Param2 │ Param3 │
 * └─────────────────────────────────────────┴────────┴────────┴────────┘
 *                 interrupt handler content | thread content |
 * 
 * now, interrupt handler starts to backup registers:
 * vector, ds, es, fs, gs, EAX, ECX, EDX, EBX, ESP, EBP, ESI, and EDI
 * not all handler pushed a vector, so we padded one to make stack
 * has same size.
 * 
 * each occupies 4 bytes, total: 52 bytes:
 * ┌─────────────────────────────────────────────┬─────┬─────┬────┬─────┐
 * │                                             │ EDI │ ... │ ds │ vct │
 * └─────────────────────────────────────────────┴─────┴─────┴────┴─────┘
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 */

class thread_t
{
private:

public:
    


};

class task_mgr
{
private:
    static queue_t<pcb_t> s_ready_queue;
    static queue_t<pcb_t> s_all_queue;
public:
    static void   scheduler(uint32_t no);
    static void   init();
    static pcb_t* current_pcb();
private:
    task_mgr() {};

};

ns_lite_kernel_lib_end