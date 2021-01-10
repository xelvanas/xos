#include <memory.h>
#include <ards.h>
#include <string.h>

ns_lite_kernel_lib_begin

//---------------------------------------------------------------------------
// Memory Pool

bool mem_pool::init(
    void*    buf,
    uint32_t bufsz,
    uint32_t addr,
    uint32_t len)
{
    if(buf   == nullptr ||
       bufsz == 0       ||
       len   == 0)
    {
        return false;
    }

    // dbg_mhl("buf:",   (uint32_t)buf);
    // dbg_mhl("bufsz:", (uint32_t)bufsz);

    // dbg_mhl("addr:", (uint32_t)addr);
    // dbg_mhl("len:",  (uint32_t)len);
    // while(1);


    // bit count
    uint32_t nbits = bufsz * _bmp.BIT_LENGTH;
    
    // page count
    uint32_t npgs  = len / PAGE_SIZE;

    // page count > bit count
    // bitmap cannot map whole memory area
    // init failed.
    if(npgs > nbits) {
        return false;
    }

    // round it up
    nbits = npgs + _bmp.BIT_SUPREMUM;

    // cut extra buf off to prevent bitmap from looking into
    // meaningless block.
    _bmp.reset((uint8_t*) buf, nbits/_bmp.BIT_LENGTH);

    // mark rest bit used.
    _bmp.set(npgs, _bmp.total_bits() - npgs, true);

    _start_addr = addr;
    return true;
}

void* mem_pool::alloc(uint32_t cnt)
{
    uint32_t idx = _bmp.find(0, _bmp.total_bits(), false, cnt);
    if(idx != _bmp.INVALID_INDEX) {
        _bmp.set(idx, cnt, true);
        return (void*)(_start_addr + idx * PAGE_SIZE);
    }
    return nullptr;    
}

void mem_pool::free(void* addr, uint32_t cnt)
{
    // address out of range
    if((_start_addr > (uint32_t)addr) ||
       (_start_addr + _bmp.total_bits() * PAGE_SIZE < (uint32_t)addr))
    {
        return;
    }
    uint32_t idx = ((uint32_t)addr - _start_addr)/PAGE_SIZE;
    _bmp.set(idx, cnt, false);
}
//
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// Memory Manager
mem_pool mem_mgr::_kp_pool;
mem_pool mem_mgr::_kv_pool;

void mem_mgr::init()
{
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
     * kernel at least needs 4 pools:
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
    uint32_t mem_size   = phsy_size/2 - (0x00400000 - phys_addr);
    uint32_t start_addr = 0x00400000;
    memset((void*)KER_P_BMP_BUF, 0, 0x1000);

    _kp_pool.init(
        (void*)KER_P_BMP_BUF,
        0x1000,
        start_addr,
        mem_size);
}

void* mem_mgr::alloc(page_type_t pt, uint32_t cnt)
{
    if(cnt == 0)
        return nullptr;

    if(pt == PT_KERNEL)
    {
        return _kp_pool.alloc(cnt);
    }
    return nullptr;
}


void mem_mgr::__inner_detect_valid_mem(
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