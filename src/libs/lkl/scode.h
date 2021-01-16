#pragma once
#include <lkl.h>
#include <bit.h>

ns_lite_kernel_lib_begin

// keyboard modifier states
class kbdms_t
{
private:
    uint8_t _md_state  = 0;
public:
    enum
    {
        S_L_ALT        = 0x01,
        S_R_ALT        = 0x02,
        S_L_CTRL       = 0x04,
        S_R_CTRL       = 0x08,
        S_L_SHIFT      = 0x10,
        S_R_SHIFT      = 0x20,
        S_CAPS_LOCK    = 0x40,
        S_EXT_CODE     = 0x80
    };

    enum
    {
        SC_ALT         = 0x38,
        SC_CTRL        = 0x1D,
        SC_L_SHIFT     = 0x2A,
        SC_R_SHIFT     = 0x36,
        SC_CAPS_LOCK   = 0x3A,
        SC_EXT_CODE    = 0xE0,
    };

    enum
    {
        MASK_MAKE_CODE = 0x7F,
        MASK_KEY_UP    = 0x80
    };

    kbdms_t() {

    }

    kbdms_t(uint8_t md) : _md_state(md) {

    }

    inline bool
    update(uint8_t scode)
    {
        if(scode == SC_EXT_CODE) {
            lkl::bit_set(_md_state, S_EXT_CODE, true);
            return true;
        }
        
        bool ret   = false;
        bool kdown = __inner_is_key_down(scode);
        switch(__innert_make_code(scode)) 
        {
            case SC_ALT:
                {
                    lkl::bit_set(
                        _md_state,
                        has_ext_code() ?
                        (uint8_t)S_R_ALT :
                        (uint8_t)S_L_ALT,
                        kdown
                    );
                    ret = true;
                    break;
                } 
            case SC_CTRL:
                {
                    lkl::bit_set(
                        _md_state,
                        has_ext_code() ?
                        (uint8_t)S_R_CTRL :
                        (uint8_t)S_L_CTRL,
                        kdown
                    );
                    ret = true;
                    break;
                }
            case SC_L_SHIFT:
                {
                    lkl::bit_set(_md_state, S_L_SHIFT, kdown);
                    ret = true;
                }
                break;
            case SC_R_SHIFT:
                {
                    lkl::bit_set(_md_state, S_R_SHIFT, kdown);
                    ret = true;
                }
                break;
            case SC_CAPS_LOCK:
                {
                    if(kdown) {
                        lkl::bit_toggle(_md_state, S_CAPS_LOCK);
                    }
                    ret = true;
                }
                break;
        }

        if(ret == true &&
           has_ext_code())
        {
            reset_ext_code();
        }
        return ret;
    }

    inline void
    reset(uint8_t ns = 0) {
        _md_state = ns;
    }

    inline bool
    is_shift_down() const {
        return is_lshift_down() || is_rshift_down();
    }

    inline bool
    is_lshift_down() const {
        return lkl::bit_test(_md_state, S_L_SHIFT);
    }

    inline bool
    is_rshift_down() const {
        return lkl::bit_test(_md_state, S_R_SHIFT);
    }

    inline bool
    is_alt_down() const {
        return is_lalt_down() || is_ralt_down();
    }

    inline bool
    is_lalt_down() const {
        return lkl::bit_test(_md_state, S_L_ALT);
    }

    inline bool
    is_ralt_down() const {
        return lkl::bit_test(_md_state, S_R_ALT);
    }

    inline bool
    is_ctrl_down() const {
        return is_lctrl_down() || is_rctrl_down();
    }

    inline bool
    is_lctrl_down() const {
        return lkl::bit_test(_md_state, S_L_CTRL);
    }

    inline bool
    is_rctrl_down() const {
        return lkl::bit_test(_md_state, S_R_CTRL);
    }

    inline bool
    is_caps_lock_on() const {
        return lkl::bit_test(_md_state, S_CAPS_LOCK);
    }

    inline bool
    has_ext_code() const {
        return lkl::bit_test(_md_state, S_EXT_CODE);
    }

    inline void
    reset_ext_code() {
        lkl::bit_set(_md_state, S_EXT_CODE, false);
    }

    inline
    operator uint8_t() const {
        return _md_state;
    }

    inline const kbdms_t&
    operator=(uint8_t md) {
        _md_state = md;
        return *this; 
    }

private:
    inline uint8_t
    __innert_make_code(uint8_t scode) {
        return scode & 0x7F;
    }

    inline bool
    __inner_is_key_up(uint8_t scode) {
        return lkl::bit_test(scode, MASK_KEY_UP);
    }

    inline bool
    __inner_is_key_down(uint8_t scode) {
        return !lkl::bit_test(scode, MASK_KEY_UP);
    }
};


class scode_t
{
    kbdms_t    _md;
    uint8_t    _sc;
    // static char s_keymap[][16];
public:
    enum
    {
        SC_MASK_MC      = 0x7F,
        SC_MASK_UP      = 0x80,
        SC_KEY_SPACE    = 0x39,
        SC_KEY_ENTER    = 0x1C,
        SC_KEY_DELETE   = 0x53,
        SC_KEY_LEFT     = 0x46, // with ext code, use function to test
        SC_KEY_RIGHT    = 0x4D, // with ext code
        SC_KEY_UP       = 0x48, // with ext code
        SC_KEY_DOWN     = 0x50, // with ext code
    };

    scode_t(){

    }

    scode_t(uint8_t sc, kbdms_t md)
        : _md(md),
          _sc(sc)
    {

    }

    inline void
    reset(uint8_t sc, kbdms_t md) {
        _md = md;
        _sc = sc;
    }

    inline uint8_t
    make_code() const {
        return _sc & SC_MASK_MC;
    }

    inline bool
    is_key_down() const {
        return !is_key_up();
    }

    inline bool
    is_key_up() const {
        return lkl::bit_test(_sc, SC_MASK_UP);
    }

    inline bool
    is_space() const {
        return make_code() == SC_KEY_SPACE;
    }

    inline bool
    is_enter() const {
        return make_code() == SC_KEY_ENTER;
    }

    inline bool
    is_delete() const {
        return make_code() == SC_KEY_DELETE;
    }

    inline bool
    is_arrow() const {
        if(make_code() == SC_KEY_LEFT  ||
           make_code() == SC_KEY_RIGHT ||
           make_code() == SC_KEY_UP   ||
           make_code() == SC_KEY_DOWN) {
               return _md.has_ext_code();
           }
           return false;
    }

    inline bool
    is_left_arrow() const {
        return make_code() == SC_KEY_LEFT && _md.has_ext_code();
    }

    inline bool
    is_right_arrow() const {
        return make_code() == SC_KEY_RIGHT && _md.has_ext_code();
    }

    inline bool
    is_up_arrow() const {
        return make_code() == SC_KEY_UP && _md.has_ext_code();
    }

    inline bool
    is_down_arrow() const {
        return make_code() == SC_KEY_DOWN && _md.has_ext_code();
    }


    inline kbdms_t&
    modifier() {
        return _md;
    }

    inline const kbdms_t&
    modifier() const {
        return _md;
    }

    inline uint8_t
    code() const {
        return _sc;
    }

    inline uint8_t&
    code() {
        return _sc;
    }

    inline bool
    is_shapeshifter() const {
        uint8_t c = make_code();
        return (c >= 0x02 && c <= 0x0e) || // '1' -> 'bksp'
               (c >= 0x33 && c <= 0x35) || // ',' -> '/'
               (c >= 0x27 && c <= 0x28) || // ';' -> '''
               (c >= 0x1a && c <= 0x1b) || // '[' -> ']'
               (c == 0x29);                // '`'
    }

    inline bool
    is_alphabet() const {
        uint8_t c = make_code();
        return (c >= 0x10 && c <= 0x19) || // 'q' -> 'p'
               (c >= 0x1e && c <= 0x26) || // 'a' -> 'l'
               (c >= 0x2c && c <= 0x32);   // 'z' -> 'm'
    }

    inline bool
    is_visible() const {
        return is_alphabet()  ||
               is_shapeshifter();
    }

    inline bool
    is_special() const {
        return is_delete() ||
               is_enter()  ||
               is_space();
    }

    // convert 'make code' to ascii
    char
    to_char() const;

    operator char() const { return to_char(); }

};

ns_lite_kernel_lib_end