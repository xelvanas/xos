#pragma once
#include <lkl.h>
#include <debug.h>

ns_lite_kernel_lib_begin
template<typename bits_t>
class bitmap_t
{
private:
    bits_t*  _buf      = nullptr; // buffer
    uint32_t _buf_size = 0;       // how many 'bits_t' _buf has
    uint32_t _bit_size = 0;       // bit size of buffer
    uint32_t _limit    = 0;       // bit limit, range (0, _bit_size)

public:
    enum
    {
        BIT_LENGTH    = sizeof(bits_t) * 8,
        BIT_SUPREMUM  = BIT_LENGTH - 1,
        BIT_INFIMUM   = 0,
        MAX_VALUE     = ~((bits_t)0),
        INVALID_INDEX = ~((uint32_t)0)
    };

    // default constructor
    bitmap_t() = default;

    // not support len > 0x1FFFFFFF
    // cuz max value of uint32_t is 0xFFFFFFFF
    bitmap_t(bits_t* buf, uint32_t len) :
        _buf(buf),
        _buf_size(len),
        _bit_size(len*BIT_LENGTH),
        _limit(_bit_size) {

    }

    void*
    reset(bits_t* buf, uint32_t len) {
        auto tmp  = (void*)_buf;
        _buf      = buf;
        _buf_size = len;
        _bit_size = len * BIT_LENGTH;
        _limit    = _bit_size;
        return tmp;
    }

    void*
    get_buffer() const {
        return (void*)_buf;
    }

    // get buffer size
    uint32_t
    buffer_size() const {
        return _buf_size;
    }

    uint32_t
    bit_size() const {
        return _bit_size;
    }

    bool
    limit(uint32_t limit) {
        if(limit >= 0 && limit <= bit_size()) {
            _limit = limit;
            return true;
        }
        return false;
    }

    uint32_t
    limit() const {
        return _limit;
    }

        bool
    test(uint32_t idx, bool val = true) const {
        // error index
        ASSERT(idx < bit_size() && "error idx value.");

        return ((_buf[idx / BIT_LENGTH] & bit_mask(idx)) != 0) == val;
    }

    // the units of 'idx' and 'len' are 'bit'
    bool
    test(uint32_t idx, bool val, uint32_t len) {
        for (uint32_t i = 0; i < len; ++i) {
            if (test(idx + i, val) == false)
                return false;
        }
        return true;
    }

    // set _buf[idx] = val
    void set(uint32_t idx, bool val) {
        if (idx >= bit_size()) {
            return;
        }
        auto& elem = _buf[idx / BIT_LENGTH];
        elem = val ? elem | bit_mask(idx) : elem & ~bit_mask(idx);
    }

    // set _buf[idx] -> _buf[idx+len] = val
    void set(uint32_t idx, uint32_t len, bool val) {
        if (idx >= bit_size()) {
            return;
        }

        if (idx + len > bit_size()) {
            len = bit_size() - idx;
        }

        uint32_t end = idx + len;
        while (idx < end)
        {
            if (idx % BIT_LENGTH == 0 && end - idx > BIT_LENGTH)
            {
                _buf[idx / BIT_LENGTH] = val ? MAX_VALUE : 0;
                idx += BIT_LENGTH;
            }
            else
            {
                set(idx, val);
                ++idx;
            }
        }
    }

    uint32_t count(uint32_t start, uint32_t len, bool val)
    {
        if(start  >= limit())
            return 0;

        auto end = start+len > limit() ? limit() : start+len;
        uint32_t n = 0;
        for(start; start < end; ++start) {
            if(test(start,val)) {
                ++n;
            }
        }
        return n;
    }

    uint32_t count(bool val, uint32_t cnt = 0xFFFF'FFFF)
    {
        bits_t   full =  val ? MAX_VALUE : 0;
        bits_t   empt =  ~full; 
        uint32_t tail =  limit() % BIT_LENGTH;
        uint32_t rsz  = (limit() + BIT_SUPREMUM) / BIT_LENGTH;
        uint32_t num  =  0;

        for(uint32_t idx = 0; idx < rsz; ++idx) {
            if(_buf[idx] == full) {
                num += BIT_LENGTH;
            } else if (_buf[idx] == empt) {
                continue;
            } else {
                num += count(idx*BIT_LENGTH, BIT_LENGTH, val);
            }

            if(num >= cnt) {
                return num;
            }
        }
        num += count(limit() - tail, limit(), val);
        return num;
    }

    // find contiguous values in range
    uint32_t find(uint32_t start, uint32_t end, bool val, uint32_t len)
    {
        // cannot search beyond '_limit'
        end = end > limit() ? limit() : end;

        while (start < end)
        {
            uint32_t idx = roughly_find(start, end, val);
            if (idx == INVALID_INDEX) {
                return INVALID_INDEX;
            }

            if (idx * BIT_LENGTH > start) {
                start = idx * BIT_LENGTH;
            }

            uint32_t ed  = (idx + 1) * BIT_LENGTH + len;
                     ed  = ed < end ? ed : end;

            uint32_t fit = INVALID_INDEX;
            uint32_t n   = 0;

            for (;start < ed; ++start) {

                if (test(start, val)) {
                    if (fit == INVALID_INDEX) {
                        fit = start;
                    }
                    if (++n == len) {
                        // we found enough bits
                        return fit;
                    }
                }
                else if (fit != INVALID_INDEX) {
                    fit = INVALID_INDEX; // first unfit
                    n   = 0; // count reset
                }
            }
            start = (idx + 1) * BIT_LENGTH;
        }
        return INVALID_INDEX;
    }

protected:

    bits_t
    bit_mask(uint32_t idx) const {
        return (bits_t)1 << (idx % BIT_LENGTH);
    }

    // skip those 0xFFFF/0x0000 to save time
    uint32_t roughly_find(uint32_t start, uint32_t end, bool val = true) {
        start /= BIT_LENGTH;
        end   /= BIT_LENGTH;
        if(start >= _buf_size) {
            return INVALID_INDEX;
        }

        // real size
        uint32_t rsz = (limit() + BIT_SUPREMUM) / BIT_LENGTH;

        end = end > rsz ? rsz : end;

        bits_t full = val ? 0 : MAX_VALUE;

        while (start < end) {
            if (_buf[start] != full) {
                return start;
            }
            ++start;
        }
        return INVALID_INDEX;
    }
};

ns_lite_kernel_lib_end