#pragma once
#include <lkl.h>
#include <x86/pg.h>
#include <debug.h>

ns_lite_kernel_lib_begin

class page_dir
{
private:
    uint32_t _addr;
public:
    /*
     * parameter 1: where to store 'page-directory'
     *              should be 4K aligned
     */
    page_dir(uint32_t addr)
        : _addr(addr) {
        ASSERT(__inner_4k_alignment_test(addr));
    }


private:
    inline static bool
    __inner_4k_alignment_test(uint32_t addr) {
        return addr & 0x0000'0FFF == 0;
    }
};


ns_lite_kernel_lib_end