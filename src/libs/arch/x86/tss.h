#pragma once
#include <stdint.h>
#include <x86/mmu.h>

class tss_t
{
private:
    uint16_t    _back_link,
                __rsrv_blh;
    uint32_t    _esp0;
    uint16_t    _ss0,
                __rsrv_ss0h;
    uint32_t    _esp1;
    uint16_t    _ss1,
                __rsrv_ss1h;
    uint32_t    _esp2;
    uint16_t    _ss2,
                __rsrv_ss2h;
    uint32_t    _cr3;
    uint32_t    _eip;
    uint32_t    _eflags;
    uint32_t    _eax;
    uint32_t    _ecx;
    uint32_t    _edx;
    uint32_t    _ebx;
    uint32_t    _esp;
    uint32_t    _ebp;
    uint32_t    _esi;
    uint32_t    _edi;
    uint16_t    _es,
                __rsrv_esh;
    uint16_t    _cs,
                __rsrv_csh;
    uint16_t    _ss,
                __rsrv_ssh;
    uint16_t    _ds,
                __rsrv_dsh;
    uint16_t    _fs,
                __rsrv_fsh;
    uint16_t    _gs,
                __rsrv_gsh;
    uint16_t    _ldt,
                __rsrv_ldth;
    uint16_t    _trace;
    uint16_t    _io_bmp_base;
public:
    inline void
    zeroize() {
        _back_link   = 0;
        _esp0        = 0;
        _ss0         = 0;
        // _esp1        = 0;
        // _ss1         = 0;
        // _esp2        = 0;
        // _ss2         = 0;
        _cr3         = 0;
        _eip         = 0;
        _eflags      = 0;
        _eax         = 0;
        _ecx         = 0;
        _edx         = 0;
        _ebx         = 0;
        _esp         = 0;
        _ebp         = 0;
        _esi         = 0;
        _edi         = 0;
        _es          = 0;
        __rsrv_esh   = 0;
        _cs          = 0;
        _ss          = 0;
        _ds          = 0;
        _fs          = 0;
        _gs          = 0;
        _ldt         = 0;
        _trace       = 0;
        _io_bmp_base = 0;
    }
public:
    static void
    init();
private:
    static uint32_t s_states;
};
