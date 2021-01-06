/* -----------------------------------------------------------------------
 * NOTEs:
 * 
 * 1. In this stage, we assume that 'Protected Mode' was enabled. Code is
 *    running under flat mode (32-bit).
 * 
 * 2. 'Paging' will not be enabled in this stage. The major purpose here is
 *    to build a simple File System and load the real KERNEL from it.
 * 
 * 3. We do NOT use C++ standard library because of that much of its functions
 *    and features depend on something-else that may cause serious issues.
 * 
 * 4. The only reason we use 'C++' here is making coding simpler rather than
 *    writting more complex code. So, C++ will be limitedly used here. If you
 *    are not sure what 'asm' code the compiler will generate, go figure it
 *    out or do NOT write code that way.
 * 
 * 5. No 'malloc' 'free' functions yet, 'new' and 'delete' don't even think
 *    about it. we even do NOT 'alloc/free' any memory here. because we own
 *    the whole physical memory. but there's still one catch here, some 
 *    segments are used by BIOS or other things. read 'low level memory
 *    layout' to ensure that you're not replacing important data.
 * ----------------------------------------------------------------------- */
#include <print.h>
#include <x86/io.h>

using namespace lkl;

int main() {
    print_t<def_screen_t, x86_isa> pt;
    color_t col1(color_t::B_GREEN  |
                 color_t::F_YELLOW |
                 color_t::MASK_BLINK);
    color_t col2(color_t::F_YELLOW, color_t::B_GREEN);
    pt.set_default_color(col2);
    pt.show("XOS is");
    pt.set_default_color(col1);
    pt.show(" running!");
    while(1);
    return 0;
}