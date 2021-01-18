#pragma once
#include <lkl.h>
#include <x86/pg.h>
#include <bitmap.h>


ns_lite_kernel_lib_begin

class pool_t
{
private:
    bitmap_t<uint8_t> _bmp;
    uint32_t          _base;
    uint32_t          _nfp; // free page count
public:

    pool_t()
        : _base(0),
          _nfp(0)
    {

    }

    pool_t(void*     buf,
           uint32_t  size,
           uint32_t  addr,
           uint32_t  len)
    {
        // init(buf, size, addr, len);
    }

    uint32_t page_count() const {
        return _bmp.limit();
    }

    uint32_t free_page_count() const {
        return _nfp;
    }

    uint32_t used_page_count() const {
        return _bmp.limit() - _nfp;
    }

    // provide ability to free buffer
    void*
    get_buffer() const {
        return _bmp.get_buffer();
    }

    uint32_t
    buffer_size() const {
        return _bmp.buffer_size();
    }

    void
    reset(void*     buf,        // buffer address
          uint32_t  buf_size,   // by byte
          uint32_t  base,       // base address
          uint32_t  space_size) // address space size
    {
        // bit count
        uint32_t nbits = buf_size * _bmp.BIT_LENGTH;
        
        // free page count
        _nfp  = space_size / PAGE_SIZE;

        // page count > bit count
        // bitmap cannot map whole memory area
        if(_nfp > nbits) {
            _nfp = nbits;
        }

        // set bitmap buffer
        _bmp.reset((uint8_t*)buf, buf_size);

        // cut extra buf off to prevent bitmap from looking into
        // invalid bits.
        // fill invalid bits with 1
        _bmp.set(_nfp, _bmp.bit_size() - _nfp, true);
        _bmp.limit(_nfp);

        // mark rest bit used.
        ASSERT(_bmp.count(false) == _nfp);
        ASSERT(_bmp.count(true)  == 0);

        _base = base;
    }
    
    void* alloc(uint32_t cnt)
    {
        if(cnt <= _nfp) {
            uint32_t idx = _bmp.find(0, _bmp.limit(), false, cnt);
            if(idx != _bmp.INVALID_INDEX) {
                _bmp.set(idx, cnt, true);
                _nfp -= cnt;
                return (void*)(_base + idx * PAGE_SIZE);
            }
        }
        return nullptr;  
    }

    void 
    free(void* addr, uint32_t cnt)
    {
        // address out of range
        if((_base > (uint32_t)addr) ||
           (_base + _bmp.limit() * PAGE_SIZE < (uint32_t)addr))
        {
            return;
        }

        uint32_t idx = ((uint32_t)addr - _base)/PAGE_SIZE;

        ASSERT((uint32_t)addr % PAGE_SIZE == 0 && 
               "address is not 4K aligned.");

        ASSERT(_bmp.find(idx, idx + cnt, true, cnt) == idx &&
               "cheap validity checking");

        _bmp.set(idx, cnt, false);
    }

};


ns_lite_kernel_lib_end