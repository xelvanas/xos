#pragma once
#include <lkl.h>
#include <bitmap.h>


ns_lite_kernel_lib_begin

class pool_t
{
private:
    bitmap_t<uint8_t> _bmp;
    uint32_t          _start_addr;
    uint32_t          _free_pg;
public:
    enum
    {
        PAGE_SIZE = 0x1000, // 4K one page
    };

    pool_t()
        : _start_addr(0),
          _free_pg(0)
    {

    }

    pool_t(void*     buf,
           uint32_t  bufsz,
           uint32_t  addr,
           uint32_t  len)
    {
        init(buf, bufsz, addr, len);
    }

    uint32_t page_cnt() const {
        return _bmp.limit();
    }

    uint32_t free_pg_cnt() const {
        return _free_pg;
    }

    uint32_t used_pg_cnt() const {
        return _bmp.limit() - _free_pg;
    }

    bool  init(void*     buf,
               uint32_t  bufsz,
               uint32_t  addr,
               uint32_t  len)
    {
        if(buf   == nullptr ||
           bufsz == 0       ||
           len   == 0)
        {
            return false;
        }

        // bit count
        uint32_t nbits = bufsz * _bmp.BIT_LENGTH;
        
        // free page count
        _free_pg  = len / PAGE_SIZE;

        // page count > bit count
        // bitmap cannot map whole memory area
        // init failed.
        if(_free_pg > nbits) {
            return false;
        }

        // round it up
        nbits = _free_pg + _bmp.BIT_SUPREMUM;

        // set bitmap buffer
        _bmp.reset((uint8_t*)buf, bufsz);

        // cut extra buf off to prevent bitmap from looking into
        // invalid bits.
        // fill invalid bits with 1
        _bmp.set(_free_pg, _bmp.bit_size() - _free_pg, true);
        _bmp.limit(_free_pg);

        // mark rest bit used.
        ASSERT(_bmp.count(false) == _free_pg);
        ASSERT(_bmp.count(true)  == 0);

        _start_addr = addr;
        return true;
    }
    
    void* alloc(uint32_t cnt)
    {
        if(cnt <= _free_pg) {
            uint32_t idx = _bmp.find(0, _bmp.limit(), false, cnt);
            if(idx != _bmp.INVALID_INDEX) {
                _bmp.set(idx, cnt, true);
                _free_pg -= cnt;
                return (void*)(_start_addr + idx * PAGE_SIZE);
            }
        }
        return nullptr;  
    }

    void  free(void* addr, uint32_t cnt)
    {
        // address out of range
        if((_start_addr > (uint32_t)addr) ||
        (_start_addr + _bmp.limit() * PAGE_SIZE < (uint32_t)addr))
        {
            return;
        }

        uint32_t idx = ((uint32_t)addr - _start_addr)/PAGE_SIZE;

        ASSERT((uint32_t)addr % PAGE_SIZE == 0 && 
               "address is not 4K aligned.");

        ASSERT(_bmp.find(idx, idx + cnt, true, cnt) == idx &&
               "cheap validity checking");

        _bmp.set(idx, cnt, false);
    }
};


ns_lite_kernel_lib_end