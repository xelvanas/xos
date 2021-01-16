#pragma  once
#include <stdint.h>
#include <bit.h>
#include <string.h>
#include <debug.h>



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
                        is_ext_code() ?
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
                        is_ext_code() ?
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
           is_ext_code())
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
    is_shift_down() {
        return is_lshift_down() || is_rshift_down();
    }

    inline bool
    is_lshift_down() {
        return lkl::bit_test(_md_state, S_L_SHIFT);
    }

    inline bool
    is_rshift_down() {
        return lkl::bit_test(_md_state, S_R_SHIFT);
    }

    inline bool
    is_alt_down() {
        return is_lalt_down() || is_ralt_down();
    }

    inline bool
    is_lalt_down() {
        return lkl::bit_test(_md_state, S_L_ALT);
    }

    inline bool
    is_ralt_down() {
        return lkl::bit_test(_md_state, S_R_ALT);
    }

    inline bool
    is_ctrl_down() {
        return is_lctrl_down() || is_rctrl_down();
    }

    inline bool
    is_lctrl_down() {
        return lkl::bit_test(_md_state, S_L_CTRL);
    }

    inline bool
    is_rctrl_down() {
        return lkl::bit_test(_md_state, S_R_CTRL);
    }

    inline bool
    is_caps_lock_on() {
        return lkl::bit_test(_md_state, S_CAPS_LOCK);
    }

    inline bool
    is_ext_code() {
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
        SC_MASK_MC  = 0x7F,
        SC_MASK_UP  = 0x80
    };

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
    is_key_down() {
        return !lkl::bit_test(_sc, SC_MASK_UP);
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

    // convert 'make code' to ascii
    char
    to_char() const;
};

class keyboard
{
private:
    static uint8_t s_states;
    static kbdms_t s_modifiers;
public:
    enum
    {
        KBS_INITED  = 0x01,
        KBD_PORT    = 0x60,
    };
    static void
    init();

    static bool
    is_initialized() {
        return lkl::bit_test(s_states, KBS_INITED);
    }
    
    static void
    keyboard_handler(uint32_t vct);

private:
    keyboard() = delete;
};
