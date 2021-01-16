#pragma  once
#include <stdint.h>
#include <bit.h>
#include <string.h>
#include <debug.h>
#include <scode.h>

class keyboard
{
private:
    static uint8_t      s_states;
    static lkl::kbdms_t s_modifiers;
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
