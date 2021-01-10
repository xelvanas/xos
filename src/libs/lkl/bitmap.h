#pragma once
#include <lkl.h>
#include <debug.h>

ns_lite_kernel_lib_begin

template<typename bits_t>
class bitmap_t
{
private:
    uint32_t    _len;     // how many 'bits_t' _buf has
    bits_t*     _buf;     // buffer
public:
    class result_t
    {
    public:
        bool     _val;
        uint32_t _1st_fit;
        uint32_t _1st_unfit;

        result_t(bool val) :
            _val(val),
            _1st_fit(INVALID_INDEX),
            _1st_unfit(INVALID_INDEX)
        {

        }

        bool both_valid() const {
            return _1st_fit   != INVALID_INDEX &&
                   _1st_unfit != INVALID_INDEX;
        }

        bool both_invalid() const {
            return _1st_fit   == INVALID_INDEX &&
                   _1st_unfit == INVALID_INDEX;
        }

        bool is_fit_valid() const {
            return _1st_fit == INVALID_INDEX;
        }

        bool is_unfit_valid() const {
            return _1st_unfit == INVALID_INDEX;
        }
    };

public:
    enum
    {
        BIT_LENGTH      = sizeof(bits_t) * 8,
        BIT_SUPREMUM    = BIT_LENGTH - 1,
        BIT_INFIMUM     = 0,
        MAX_VALUE       = ~((bits_t)0),
        INVALID_INDEX   = ~((uint32_t)0)
    };

    bitmap_t() :
        _buf(nullptr),
        _len(0)
    {

    }

    // not support len > 0x1FFFFFFF
    // cuz max value of uint32_t is 0xFFFFFFFF
    bitmap_t(bits_t* buf, uint32_t len) :
        _buf(buf),
        _len(len)
    {

    }

    void reset(bits_t* buf, uint32_t len) {
        _buf = buf;
        _len = len;
    }

    uint32_t total_bits() const {
        return _len * sizeof(bits_t) * 8;
    }


    bits_t mask(uint32_t idx) const {
        return (bits_t)1 << (idx % BIT_LENGTH);
    }

    bool test(uint32_t idx, bool val = true) const {

        dbg_mhl("idx:", (uint32_t)idx);
        dbg_mhl("total:", (uint32_t)total_bits());
        ASSERT(idx < total_bits() && "error idx value.");

        if (idx >= total_bits()) {
            // will use 'ASSERT' instead or throw an exception once 
            // we support 
            return false;
        }

        return ((_buf[idx/BIT_LENGTH] & mask(idx)) != 0) == val;
    }

    // the units of 'idx' and 'len' are 'bit'
    bool test(uint32_t idx, bool val, uint32_t len) {
        for (uint32_t i = 0; i < len; ++i) {
            if (test(idx + i, val) == false)
                return false;
        }
        return true;
    }

    // set _buf[idx] = val
    void set(uint32_t idx, bool val) {
        if (idx >= total_bits()) {
            return;
        }
        auto& elem = _buf[idx / BIT_LENGTH];
        elem = val ? elem | mask(idx) : elem & ~mask(idx);
    }

    // set _buf[idx] -> _buf[idx+len] = val
    void set(uint32_t idx, uint32_t len, bool val) {
        if (idx >= total_bits()) {
            return;
        }

        if (idx + len > total_bits()) {
            len = total_bits() - idx;
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

    // find a specific value from range
    result_t find(uint32_t start, uint32_t end, bool val)
    {
        result_t rst(val);
        for (; start < end; ++start) {

            if (test(start, val)) {
                if (rst._1st_fit == INVALID_INDEX) {
                    rst._1st_fit = start;
                }
            } else {
                if (rst._1st_unfit == INVALID_INDEX) {
                    rst._1st_unfit = start;
                }
            }

            if (rst._1st_unfit != INVALID_INDEX &&
                rst._1st_fit   != INVALID_INDEX) {
                break;
            }
        }
        return rst;
    }

    // find contiguous values in range
    uint32_t find(uint32_t start, uint32_t end, bool val, uint32_t len)
    {
        uint32_t ridx = roughly_find(start, end, val);
        if (ridx == INVALID_INDEX) {
            return INVALID_INDEX;
        }

        if (ridx * BIT_LENGTH > start) {
            start = ridx * BIT_LENGTH;
        }
        uint32_t ed = (ridx + 1) * BIT_LENGTH + len;
        ed = ed < end ? ed : end;

        // want one, searched many
        // WILL OPTIMIZE LATER
        auto rst = find(start, ed, val);

        if (rst._1st_fit != INVALID_INDEX)
        {
            if (rst._1st_unfit < rst._1st_fit) {
                return find(rst._1st_fit, end, val, len);
            }
            ed = rst._1st_unfit == INVALID_INDEX ?
                 ed : rst._1st_unfit;

            if (ed - rst._1st_fit >= len)
            {
                return rst._1st_fit;
            }
        }
        return find(ed, end, val, len);
    }

    // skip those 0xFFFF/0x0000 to save time
    uint32_t roughly_find(uint32_t start, uint32_t end, bool val = true) {
        start /= BIT_LENGTH;
        end   /= BIT_LENGTH;
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