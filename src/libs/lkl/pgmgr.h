#pragma once
#include <lkl.h>
#include <x86/pg.h>

ns_lite_kernel_lib_begin

class pg_mgr
{
public:
    

public:
    static bool
    create_page_dir(
        uint32_t adr,   // physical mem address to store page-directory
        uint32_t ims,   // identity mapped space, unit: 4M (1 PDE)
        uint32_t uss,   // user space, unit: 4M (1 PDE)
        uint32_t kns
    );   // kernel space, unit: 4M (1 PDE)

    // virtual address allocation
    // inline static void*
    // valloc(uint32_t cnt) {
    //     return nullptr;
    // }

    // given a virtual address, calculate the virtual address of its PDE
    static inline pde_t*
    calc_pde_va(uint32_t vaddr) {
        // 1. point twice PDE start
        // 2. add PDE offset * 4 (each index occupies 4 bytes)
        return (pde_t*)((0xfffff000) + 
        (pde_t::get_pde_index(vaddr)<<2));
    }

    // give a virtual address, calculate the virtual address of its PTE
    static inline pte_t*
    calc_pte_va(uint32_t vaddr) {
        return (pte_t*)(0xffc00000 +         // point to PDE:1023
        ((vaddr & 0xffc00000) >> 10) +       // PDE as PTE (to locate PDE)
        (pte_t::get_pte_index(vaddr) << 2)); // real PTE offset
    }

    static inline void
    map_v2p(
        uint32_t vaddr,
        uint32_t paddr
    );

private:

};


ns_lite_kernel_lib_end