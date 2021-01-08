/* ---------------------------------------------------------------------------
 * NOTEs:
 * 
 * 1. At this stage, we assume that 'Protected Mode' was enabled. code is
 *    running under flat mode (32-bit).
 * 
 * 2. [RETRACTED]'Paging' will not be enabled at this stage. The major
 *    purpose here is to build a simple File System and load the real
 *    KERNEL from it.
 * 
 *    [UPDATE]: I didn't know that new partition tools use 1MB alignment
 *    which leave us 2047 sectors available space. before this, there was
 *    only 63 usable sectors, I cannot store too much code in it.
 *    fortunately, there is no more 63 sector limit, I'd like to write
 *    more features at this stage. :-)
 *    
 * 3. We do NOT use C++ standard library because of that much of its
 *    functions and features depend on something-else that may cause 
 *    serious issues.
 * 
 * 4. The only reason we use 'C++' here is making coding simpler rather
 *    than writting more complex code. So, C++ will be limitedly used
 *    here. If you are not sure what 'asm' code the compiler will 
 *    generate, go figure it out or do NOT write code that way.
 * 
 * 5. No 'malloc' 'free' functions yet, 'new' and 'delete' don't even
 *    think about it. we even do NOT 'alloc/free' any memory here.
 *    because we own the whole physical memory. but there's still one
 *    catch here, some segments are used by BIOS or other things. read
 *    'low level memory layout' to ensure that you're not replacing
 *    important data which BIOS used before.
 *
 * ------------------------------------------------------------------------ */
#include <print.h>
#include <x86/io.h>
#include <x86/paging.h>

using namespace lkl;

void msg_welcome() {
    print_t<def_screen_t, x86_isa> pt;
    color_t col1(color_t::B_GREEN  |
                 color_t::F_YELLOW |
                 color_t::MASK_BLINK);
    color_t col2(color_t::F_YELLOW, color_t::B_GREEN);
    pt.set_default_color(col2);
    pt.show("welcome to ");
    pt.set_default_color(col1);
    pt.show("XOS!\n");
}

void msg_paging_enabled() {
    print_t<screen_t<80, 25, 0xC00B8000>, x86_isa> prt;
    color_t col(color_t::B_GREEN | color_t::F_WHITE);
    
    prt.set_default_color(col);
    prt.show("Paging Enabled.\n");
}

void enable_paging(uint32_t addr) {
    const uint32_t entry_num = 1024;

    pde_t pde;
    pde.present(false); // not present yet
    pde.sup(true); // user-mode accesses disallowed
    pde.ro(false); // not read-only
    
    // 'page directory' address is 'addr'(def:0x00100000)
    // 'page directory' has 1024 PDEs
    page_dir_t pd((pge_t*)addr, entry_num);
    pd.fill(0, entry_num, pde, 0);

    pde.present(true);

    // first 'page table' address is at 0x00101000
    // 4096 bytes = 1024 32-bits
    // it needs 4 PTEs to make first 16MB physical memory identity-mapped
    // because each pte can map 4MB physical memory:
    // 4K/page * 1024 entries = 4MB
    // also map 16MB to 0xC0000000-0xC0FFFFFF
    pde.address(addr + 0x1000);
    pd.fill(0,   4,   pde, 0x1000);
    pd.fill(768, 772, pde, 0x1000);

    pde.address(addr + 0x5000);
    pd.fill(772, 1022, pde, 0x1000);

    // last pde points to 'page directory' itself.
    pde.address(addr);
    pd[1023] = pde;
    
    page_tbl_t pt(nullptr, 0);
    pte_t pte;
    pte.present(true);
    pte.sup(true);
    pte.ro(false);

    auto map_pte_on_pa = [&](pte_t* dst, uint32_t phys_addr) {
        pt.reset((pte_t*)dst, entry_num);
        pte.address(phys_addr);
        pt.fill(0, entry_num, pte, 0x1000);
    };

    // map PTEs on physical addresses
    for(uint32_t i = 0; i < 4; ++ i) {
        map_pte_on_pa((pte_t*)(pd[i].address()), i*0x00400000);
    }
    
    // CR3 has two attributes
    // but its default value could be zeroes
    x86_isa::set_cr3((uint32_t)addr);

    // enable paging
    x86_isa::set_cr0(x86_isa::get_cr0() | 0x80000000);
}

int main() {
    msg_welcome();
    enable_paging(0x00100000);
    msg_paging_enabled();

    while(1);
    return 0;
}