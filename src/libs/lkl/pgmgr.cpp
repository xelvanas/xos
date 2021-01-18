#include <pgmgr.h>
#include <memory.h>

ns_lite_kernel_lib_begin

bool
pg_mgr::create_page_dir(
    uint32_t adr,   // physical mem address to store page-directory
    uint32_t ims,   // identity mapped space, unit: 4M (1 PDE)
    uint32_t uss,   // user space, unit: 4M (1 PDE)
    uint32_t kns)
{

    // spaces can have gaps between each other but cannot 
    // be overlapped. 
    if(uss == 0 || kns == 0) {
        // zero size user/kernel space is disallowed.
            return false;
    }

    // too much entries
    if(ims + uss + kns > PD_ENT_NUM)
        return false;

    // kernel space cannot be lesser than identity mapped space
    if(kns < ims)
        return false;
    
    // const auto pde_span = PAGE_SIZE * PT_ENT_NUM;
    page_dir_t pd((pge_t*)adr, PD_ENT_NUM);
    uint32_t offset = 0;
    pge_t pge;
    
    // pge.present(true); // not present yet
    pge.usr(false);     // user-mode accesses disallowed
    pge.writable(true);     // not read-only
    pge.usr(false); 

    if(ims != 0) {
        pd.fill(offset, ims, pge);
        offset += ims;
    }

    // user space
    pge.usr(false);

    if(uss) {
        pd.fill(offset, offset+uss, pge, 0);
        offset += uss;
    }

    // kernel space
    pge.usr(true);
    if(kns) {
        pd.fill(offset, offset + kns, pge);
    }

    // special case
    // last pde points 'page-directory'
    pge.address(adr);
    pge.present(true);
    pd[PD_ENT_NUM-1] = pge;
    return true;
}

inline void
pg_mgr::map_v2p(
    uint32_t vaddr,
    uint32_t paddr)
{
    auto pde = calc_pde_va(vaddr);
    
    if(pde->present() == false) {
        // always use kernel space to save PTE
        auto pt = mem_mgr::alloc_phys_page(1);
        ASSERT(pt != 0 && "error: cannot allocate physical page");
        memset(pt, 0, PAGE_SIZE);
        pde->address((uint32_t)pt);
        pde->present(true);
        // note: cannot use physical address (pg) to access pte directly
    }
    auto pte = calc_pte_va(vaddr);
    pte->present(true);
    pte->address(paddr);
    pte->present(true);
}

ns_lite_kernel_lib_end