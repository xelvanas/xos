#pragma once
#include <lkl.h>
#include <string.h>
#include <queue.h>
#include <debug.h>
#include <bit.h>

ns_lite_kernel_lib_begin

class thread_t
{
    friend class task_mgr;
public:
    enum state_t : uint32_t
    {
        TS_UNINITED = 0x0000'0000,
        TS_RUNNING  = 0x0000'0001,
        TS_READY    = 0x0000'0002,
        TS_WAITING  = 0x0000'0003,
        TS_BLOCKED  = 0x0000'0004,
        TS_HANGING  = 0x0000'0005
    };

    enum
    {
        TH_NAME_LEN = 0x10,
        TH_MAGIC    = 0xDEAD'dead,
        TH_MASK     = 0xFFFF'F000
    };

    using tnode = qnode_t<thread_t>;
private:
    uint32_t*   _kstack;
    uint32_t    _tid;
    state_t     _state;
    uint32_t    _base_prior;
    uint32_t    _prior;
    // name is not necessary, but could make debugging easier.
    char        _name[TH_NAME_LEN];
    tnode       _node;
    tnode       _anode;

    // _magic is the tcb keeper, should always be the last member
    // of thread_t.
    uint32_t    _magic;

public:
// -----------------------------------------------------------------------
// read-only 
// -----------------------------------------------------------------------
    inline bool
    is_running() const {
        return _state == TS_RUNNING;
    }

    inline bool
    is_ready() const {
        return _state == TS_READY;
    }

    inline bool
    is_waiting() const {
        return _state == TS_WAITING;
    }

    inline bool
    is_blocked() const {
        return _state == TS_BLOCKED;
    }

    inline bool
    is_hanging() const {
        return _state == TS_HANGING;
    }

    inline state_t
    state() const {
        return _state;
    }

    inline uint32_t
    tid() const {
        return _tid;
    }

    inline uint32_t*
    kstack() const {
        return _kstack;
    }

    inline void*
    kstack_addr() const {
        return (void*)&_kstack;
    }

    inline const char*
    name() const {
        return _name;
    }

    inline tnode&
    node() {
        return _node;;
    }

    inline const tnode&
    node() const {
        return _node;;
    }

    inline tnode*
    node_ptr() {
        return &_node;;
    }

    inline tnode&
    anode() {
        return _anode;;
    }

    inline const tnode&
    anode() const {
        return _anode;;
    }

    inline tnode*
    anode_ptr() {
        return &_anode;;
    }

    inline uint32_t
    prior() const {
        return _prior;
    }

    inline uint32_t
    base_prior() const {
        return _base_prior;
    }

    inline uint32_t
    magic() const {
        return _magic;
    }

    inline bool
    is_magic_dashed() const {
        return _magic != TH_MAGIC;
    }

protected:
    inline void
    tid(uint32_t id) {
        _tid = id;
    }

    inline void
    state(state_t s) {
        _state = s;
    }

    inline void 
    kstack(uint32_t* ks) {
        _kstack = ks;
    }

    inline void
    name(const char* name) {
        //ASSERT(name != nullptr && strlen(name) < TH_NAME_LEN);
        strcpy_s(_name, TH_NAME_LEN, name);
    }

    inline void
    prior(uint32_t pr) {
        _prior = pr;
    }

    inline uint32_t
    dec_prior() {
        return --_prior;
    }

    inline void
    reset_prior() {
        _prior = _base_prior;
    }

    inline void
    base_prior(uint32_t bpr) {
        _base_prior = bpr;
    }

    inline void
    cast_magic() {
        _magic = TH_MAGIC;
    }
    
private:
    // now allow to instantialize
    thread_t() = delete;
};

ns_lite_kernel_lib_end