#include <pgmgr.h>
#include <memory.h>
#include <ards.h>

ns_lite_kernel_lib_begin
/* each bit maps 4K memory.
 * 4K = 4096 bytes * 8 = 32768 bits
 * 32768 bits * 4K = 128 MB
 * 
 * 'boot.bin' stored ARDS' information at 0x800
 * 
 * 0x01000 - 0x9F000 can be reused as pool buffers.
 * 
 * physical memory will be simpely split into 2 even pieces.
 * kernel and user each has one piece. > Luffy: (＃°Д°)?
 * 
 * Notice:
 * 'LOADER' already loaded into 0x00300000-0x003FFFFF
 */
bool
pg_mgr::init() {
    if(is_inited())
        return true;

    uint32_t addr = 0;
    uint32_t len  = 0;
    __inner_detect_valid_mem(addr, len);
    ASSERT(len > 0 && "no available physical memory!");

    if(len == 0)
        return false;
    auto     klen  = (len/2) & MASK_H20_BITS;
    auto     pgs   = __inner_calc_pages_for_buffer(klen);

    uint32_t ofst  = 0x1000;

    _k_pool.reset(
        (void*)ofst,
         pgs*PAGE_SIZE,
         addr,
         klen
    );
    
    ofst += pgs*PAGE_SIZE;
    addr += klen;

    auto ulen = (len - klen) & MASK_H20_BITS;
         pgs  = __inner_calc_pages_for_buffer(ulen);

    _u_pool.reset(
        (void*)ofst,
         pgs*PAGE_SIZE,
         addr,
         ulen
    );
    _inited = true;
    return _inited;
}

void
pg_mgr::__inner_detect_valid_mem(
    uint32_t& addr,
    uint32_t& len)
{
    addr = 0;
    len  = 0;

    uint32_t num  = *(uint32_t*)ards_t::ARDS_NUMB_ADDR;
    auto     ards =  (ards_t*)ards_t::ARDS_DATA_ADDR;

    for(uint32_t i = 0; i < num;++i) {
        if(ards[i].is_usable() && ards[i].length() > len) {
            addr = (uint32_t)ards[i].address();
            len  = (uint32_t)ards[i].length();
        }
    }
}

void*
pg_mgr::alloc(
    pgt_t type,
    uint32_t cnt)
{
    
    if(cnt == 0)
        return nullptr;
    
    lock_guard al(_lock);

    if(type == PT_KERNEL) {
        return _k_pool.alloc(cnt);
    } else {
        return _u_pool.alloc(cnt);
    }
    return nullptr;
}


// bool
// pg_mgr::create_page_dir(
//     uint32_t adr,   // physical mem address to store page-directory
//     uint32_t ims,   // identity mapped space, unit: 4M (1 PDE)
//     uint32_t uss,   // user space, unit: 4M (1 PDE)
//     uint32_t kns)
// {

//     // spaces can have gaps between each other but cannot 
//     // be overlapped. 
//     if(uss == 0 || kns == 0) {
//         // zero size user/kernel space is disallowed.
//             return false;
//     }

//     // too much entries
//     if(ims + uss + kns > PD_ENT_NUM)
//         return false;

//     // kernel space cannot be lesser than identity mapped space
//     if(kns < ims)
//         return false;
    
//     // const auto pde_span = PAGE_SIZE * PT_ENT_NUM;
//     page_dir_t pd((pge_t*)adr, PD_ENT_NUM);
//     uint32_t offset = 0;
//     pge_t pge;
    
//     // pge.present(true); // not present yet
//     pge.usr(false);     // user-mode accesses disallowed
//     pge.writable(true);     // not read-only
//     pge.usr(false); 

//     if(ims != 0) {
//         pd.fill(offset, ims, pge);
//         offset += ims;
//     }

//     // user space
//     pge.usr(false);

//     if(uss) {
//         pd.fill(offset, offset+uss, pge, 0);
//         offset += uss;
//     }

//     // kernel space
//     pge.usr(true);
//     if(kns) {
//         pd.fill(offset, offset + kns, pge);
//     }

//     // special case
//     // last pde points 'page-directory'
//     pge.address(adr);
//     pge.present(true);
//     pd[PD_ENT_NUM-1] = pge;
//     return true;
// }

// inline void
// pg_mgr::map_v2p(
//     uint32_t vaddr,
//     uint32_t paddr)
// {
//     auto pde = calc_pde_va(vaddr);
    
//     if(pde->present() == false) {
//         // always use kernel space to save PTE
//         auto pt = mem_mgr::alloc_phys_page(1);
//         ASSERT(pt != 0 && "error: cannot allocate physical page");
//         memset(pt, 0, PAGE_SIZE);
//         pde->address((uint32_t)pt);
//         pde->present(true);
//         // note: cannot use physical address (pg) to access pte directly
//     }
//     auto pte = calc_pte_va(vaddr);
//     pte->present(true);
//     pte->address(paddr);
//     pte->present(true);
// }

ns_lite_kernel_lib_end