#include <x86/asm.h>
#include <x86/io.h>
#include <stdint.h>
#include <print.h>
#include <debug.h>

using namespace lkl;

static print_t<def_screen_t, x86_io> print;

void panic_spin(
    const char* filename,
	int line,
	const char* func,
	const char* condition)
{
    // YOU CAN CHECK OUT ANY TIME YOU LIKE    
    x86_asm::turn_interrupt_off();
 
    print.set_default_color(color_t::B_GREEN | color_t::F_YELLOW);
    print.get_cursor().update(0);
    print.get_screen().fill(0, 480, col_char_t(0, color_t::B_GREEN));

    print.show("--------------------------------- FATAL"
    " ERROR ---------------------------------\n");
    print.show("File: "); print.show(filename); print.line_feed();
    print.show("Line: "); print.show(line); print.line_feed();
    print.show("Function: "); print.show(func); print.line_feed();
    print.show("Condition: "); print.show(condition); print.line_feed();
    print.show("----------------------------------------"
    "---------------------------------------\n");

    // BUT YOU CAN NEVER LEAVE!
    while(true);
}

void dbg_msg(const char* msg, uint8_t col) {
    color_t old = print.set_default_color(col);
    print.show(msg);
    print.set_default_color(old);
}

void dbg_hex(uint32_t val, uint8_t col) {
    color_t old = print.set_default_color(col);
    print.hex(val);
    print.set_default_color(old);
}
void dbg_num(uint32_t val, uint8_t col = 0x07) {
    color_t old = print.set_default_color(col);
    print.show(val);
    print.set_default_color(old);
}


void dbg_ln() {
    print.line_feed();
}
void dbg_mdl(const char* msg, uint32_t val, uint8_t col) {
    dbg_msg(msg, col);
    dbg_num(val, col);
    dbg_ln();
}

void dbg_mhl(const char* msg, uint32_t val, uint8_t col) {
    dbg_msg(msg, col);
    dbg_hex(val, col);
    dbg_ln();
}
