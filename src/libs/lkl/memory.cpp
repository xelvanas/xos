#include <memory.h>
#include <ards.h>
#include <string.h>

ns_lite_kernel_lib_begin

// pool_t phys_mem_mgr::_kp_pool;

//---------------------------------------------------------------------------
// Memory Manager
pool_t mem_mgr::_kp_pool;
pool_t mem_mgr::_kv_pool;
lock_t mem_mgr::_lock;
void mem_mgr::init()
{
    lock_guard al(_lock);

    uint32_t phys_addr = 0;
    uint32_t phsy_size = 0;
    __inner_detect_valid_mem(phys_addr, phsy_size);
    
    // no available physical memory?
    ASSERT(phsy_size > 0 && "no available physical memory!");

    /* each bit maps 4K memory.
     * 4K = 4096 bytes * 8 = 32768 bits
     * 32768 bits * 4K = 128 MB
     * 
     * mem_mgr already has 'ARDS' information, 0x0800 - 0x9F000 can 
     * be reused.
     * 
     * kernel needs at least 4 pools:
     * kernel physical memory pool: 0x0800 - 0x1800
     * kernel virtual address pool: 0x1800 - 0x2800
     * user   physical memory pool: 0x2800 - 0x3800
     * user   virtual address pool: 0x3800 - 0x4800
     * each one has 4KB bitmap
     * 
     * physical memory just be simpely split into 2 even pieces.
     * kernel and user each has one piece. > Luffy: (＃°Д°)?
     * 
     * Notice:
     * 'LOADER' already loaded at 0x00300000-0x003FFFFF
     */
    uint32_t mem_size   = phsy_size/2;
    uint32_t start_addr = phys_addr;

    memset((void*)KER_P_BMP_BUF, 0, 0x1000);

    _kp_pool.reset(
        (void*)KER_P_BMP_BUF, // bitmap buffer (physical address)
        PAGE_SIZE,            // bitmap buffer size (1 page)
        start_addr,           // physical memory starting address
        mem_size              // physical memory size
    );

    // tell _kp_pool that first 16MB has been identity-mapped in 
    // 'enable_paging' and first 1MB doesn't count in kernel pool,
    // it's a different memory segment.
    // (4096-256) = 3840 = 0xF00
    // 3840 * 4K  = 15MB
    auto addr = _kp_pool.alloc(0xF00);
    ASSERT((uint32_t)addr == start_addr);
    
    _kv_pool.reset(
        (void*)KER_V_BMP_BUF, // bitmap buffer
        PAGE_SIZE,            // bitmap buffer size
        KER_V_ADDR_START,     // starting kernel virtual address
        mem_size              // same as physical memory size
    );

    addr = _kv_pool.alloc(0x1000);
    ASSERT((uint32_t)addr == KER_V_ADDR_START);
}

void* mem_mgr::alloc(page_type_t pt, uint32_t cnt)
{
    if(cnt == 0)
        return nullptr;
    
    lock_guard al(_lock);

    if(pt == PT_KERNEL) {
        return __inner_alloc_pages(_kp_pool, _kv_pool, cnt);
    } else {
        // allocate user memory
    }
    return nullptr;
}

void*
mem_mgr::alloc_phys_page(uint32_t cnt)
{
    if(cnt == 0)
        return nullptr;

    lock_guard al(_lock);
    return _kp_pool.alloc(cnt);
}

static uint32_t
v2p(uint32_t addr)
{
    pte_t pte = addr;
    return pte.address() + (addr&pte.MASK_LO_12BITS);
}

uint32_t mem_mgr::__inner_detect_unallocated_pte(
        uint32_t vf, // first 
        uint32_t vl) // last
{
    vf =  vf & PTE_RANGE_MASK;
    vl = (vl + PTE_BOUNDARY) & PTE_RANGE_MASK;
    uint32_t num = 0;
    pde_t*   pde = nullptr;
    for(;vf < vl;) {
        pde = __inner_get_pde_v(vf);
        if(!pde->present()) {
            ++num;
        }
        vf += PTE_BOUNDARY;
    }
    return num;
}

void mem_mgr::__inner_map_virtual_on_phys(
    uint32_t vaddr,
    uint32_t paddr)
{
    auto pde = __inner_get_pde_v(vaddr);
    if(pde->present() == false) {
        // always use kernel pool to allocate PTE memory
        auto pg = _kp_pool.alloc(1);
        ASSERT(pg != 0 && "error: cannot allocate physical memory");
        memset(pg, 0, PAGE_SIZE);
        pde->address((uint32_t)pg);
        pde->present(true);
        pde->writable(true);
        // note: cannot use physical address (pg) to access pte directly
    }
    auto pte = __inner_get_pte_v(vaddr);
    pte->present(true);
    pte->address(paddr);
    pte->present(true);
}

void* mem_mgr::__inner_alloc_pages(
    pool_t& mpool,
    pool_t& vpool,
    uint32_t cnt)
{
    uint32_t vaddr = (uint32_t)vpool.alloc(cnt);
    
    uint32_t pgs   = 
    __inner_detect_unallocated_pte(
        vaddr,
        vaddr + (cnt-1) * PAGE_SIZE
    );

    if(&mpool == &_kp_pool) {
        // allocating kernel memory
        if(_kp_pool.free_page_count() < cnt + pgs) {
            // not enough memory to allocate
            vpool.free((void*)vaddr, cnt);
            return nullptr;
        }
    } else {
        // allocating user memroy
        if(_kp_pool.free_page_count() < pgs ||
           mpool.free_page_count()    < cnt) 
        {
            vpool.free((void*)vaddr, cnt);
            return nullptr;
        }
    }

    // physical address
    uint32_t paddr = 0; 
    for(uint32_t idx = 0; idx < cnt; ++idx) {
        paddr = (uint32_t)mpool.alloc(1);
        __inner_map_virtual_on_phys(vaddr, paddr);
    }
    return (void*)vaddr;
}

void mem_mgr:: __inner_detect_valid_mem(
    uint32_t& addr,
    uint32_t& len)
{
    addr = 0;
    len  = 0;

    uint32_t num = *(uint32_t*)ards_t::ARDS_NUMB_ADDR;
    auto ards    =  (ards_t*)ards_t::ARDS_DATA_ADDR;

    for(uint32_t i = 0; i < num;++i) {
        if(ards[i].is_usable() && ards[i].length() > len) {
            addr = (uint32_t)ards[i].address();
            len  = (uint32_t)ards[i].length();
        }
    }
}

//
//---------------------------------------------------------------------------

ns_lite_kernel_lib_end