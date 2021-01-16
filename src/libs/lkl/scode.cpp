#include <scode.h>
#include <string.h>

ns_lite_kernel_lib_begin


class keymap_t
{
    size_t      _len  = 0;
    const char* _chars[2] = { nullptr };
    uint8_t     _code = 0;
public:
    enum
    {
        KM_ERROR   = -1,
    };

    keymap_t(
        uint8_t     code,   
        const char* str0,
        const char* str1)
    {
        reset(code, str0, str1);
    }

    inline void
    reset(
        uint8_t     code,
        const char* str0,
        const char* str1)
    {
        if (str0 == nullptr ||
            str1 == nullptr) {
            reset();
            return;
        }
        _code     = code;
        _len      = strlen(str0);
        _chars[0] = str0;
        _chars[1] = str1;
    }

    inline void
    reset() {
        _code     = 0;
        _len      = 0;
        _chars[0] = nullptr;
        _chars[1] = nullptr;
    }

    char to_char(scode_t ch) {

        if (ch.is_visible() || ch.is_special()) {
            bool up = ch.is_shapeshifter() ?
                      ch.modifier().is_shift_down()  :
                      ch.modifier().is_shift_down()  != 
                      ch.modifier().is_caps_lock_on();
            auto buf = _chars[(uint8_t)up];
            auto rv = __inner_ret_ofst_if_in_cintvl(
            ch.make_code(),
            _code,
            _code + _len - 1);
            if (rv != KM_ERROR && rv < _len) {
                return buf[rv];
            }
        }
        return KM_ERROR;
    }

    size_t len() const {
        return _len;
    }
    
private:
    // test if value in 'closed interval'
    inline int8_t
        __inner_ret_ofst_if_in_cintvl(
            uint8_t v,  // value
            uint8_t b,  // begin
            uint8_t e)  // end
    {
        if (v >= b && v <= e) {
            return v - b;
        } else {
            return KM_ERROR;
        }
    }
};

keymap_t keymaps[] = 
{
    {0x02, "1234567890-=", "!@#$%^&*()_+"},
    {0x1a, "[]", "{}"},
    {0x27, ";'`", ":\"~"},
    {0x2b, "\\", "|"},
    {0x33, ",./", "<>?"},
    {0x01, "\033", "\033"},             // escape
    {0x0e, "\b", "\b"},
    {0x0f, "\tqwertyuiop", "\tQWERTYUIOP"},
    {0x1c, "\r", "\r"},
    {0x1e, "asdfghjkl", "ASDFGHJKL"},
    {0x2c, "zxcvbnm", "ZXCVBNM"},
    {0x37, "*", "*"},
    {0x39, " ", " "},
    {0x53, "\177", "\177"},             // delete
};

char
scode_t::to_char() const {
    auto cnt = sizeof(keymaps) / sizeof(keymap_t);
    for (auto i = 0; i < cnt; ++i) {
        auto val = keymaps[i].to_char(*this);
        if (val != -1) {
            return val;
        }
    }
    return -1;
}


ns_lite_kernel_lib_end