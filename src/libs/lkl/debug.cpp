#include <x86/io.h>
#include <stdint.h>
#include <print.h>
#include <debug.h>

void panic_spin(
    const char* filename,
	int line,
	const char* func,
	const char* condition)
{
    asm volatile("cli" : : : "memory");
    using print = lkl::print_t<lkl::def_screen_t, x86_isa>;
    print pt;
    
    pt.get_cursor().update(0);
    lkl::color_t color(lkl::color_t::B_RED);
    pt.get_screen().fill(0, 240, lkl::col_char_t::INVISIBLE_CHAR);

    pt.show("--------------------------------- FATAL"
    " ERROR ---------------------------------\n");
    pt.show("File: "); pt.show(filename); pt.line_feed();
    pt.show("Line: "); pt.show(line); pt.line_feed();
    pt.show("Function: "); pt.show(func); pt.line_feed();
    pt.show("Condition: "); pt.show(condition); pt.line_feed();
    pt.show("----------------------------------------"
    "---------------------------------------\n");

    // YOU CAN CHECK OUT ANY TIME YOU LIKE
    // BUT YOU CAN NEVER LEAVE!
    while(true);
}