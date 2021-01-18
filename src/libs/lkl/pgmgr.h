#pragma once
#include <lkl.h>
#include <lock.h>
#include <pool.h>
#include <x86/pg.h>

ns_lite_kernel_lib_begin

class pg_mgr
{
private:
    // kernel physical 
    lock_t   _lock;
    pool_t   _k_pool;
    pool_t   _u_pool;
    bool     _inited : 1 = false;
    enum
    {
        ST_INITIALIZED = 1,
        PAGE_MAP_RANGE = 0x8000000, // 128 MB
    };
public:
    enum pgt_t
    {
        PT_KERNEL = 0,
        PT_USER   = 1
    };

public:

    bool
    init();

    inline bool
    is_inited() const {
        return _inited;
    }

    void*
    alloc(pgt_t type, uint32_t cnt);

public:
    // static bool
    // create_page_dir(
    //     uint32_t adr,   // physical mem address to store page-directory
    //     uint32_t ims,   // identity mapped space, unit: 4M (1 PDE)
    //     uint32_t uss,   // user space, unit: 4M (1 PDE)
    //     uint32_t kns
    // );   // kernel space, unit: 4M (1 PDE)

    // virtual address allocation
    // inline static void*
    // valloc(uint32_t cnt) {
    //     return nullptr;
    // }

    // given a virtual address, calculate the virtual address of its PDE
    // static inline pde_t*
    // calc_pde_va(uint32_t vaddr) {
    //     // 1. point twice PDE start
    //     // 2. add PDE offset * 4 (each index occupies 4 bytes)
    //     return (pde_t*)((MASK_H20_BITS) + 
    //     (calc_pde_index((void*)vaddr) << 2));
    // }

    // // give a virtual address, calculate the virtual address of its PTE
    // static inline pte_t*
    // calc_pte_va(uint32_t vaddr) {
    //     return (pte_t*)(MASK_H10_BITS +         // point to PDE:1023
    //     ((vaddr & MASK_H10_BITS) >> 10) +       // PDE as PTE (to locate PDE)
    //     (calc_pte_index((void*)vaddr) << 2)); // real PTE offset
    // }

    // static inline void
    // map_v2p(
    //     uint32_t vaddr,
    //     uint32_t paddr
    // );

private:
    // len = 0 if failed
    static void
    __inner_detect_valid_mem(
        uint32_t& addr,
        uint32_t& len);
    static uint32_t
    __inner_calc_pages_for_buffer(uint32_t len) {
        return (len+PAGE_MAP_RANGE-1)/PAGE_MAP_RANGE;   
    }
    // static uint32_t
    // __inner_make_addr_4K_aligned(uint32_t addr) {
    //     return addr & MASK_H20_BITS;
    // }


};


ns_lite_kernel_lib_end