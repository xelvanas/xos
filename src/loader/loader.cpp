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
 * 5. No 'malloc' 'free' functions yet, 'new' and 'delete'? don't even
 *    think about it. we even do NOT 'alloc/free' any memory for now.
 *    because we own the whole physical memory. but there's still one
 *    catch, some segments are used by BIOS or other things. read
 *    'low level memory layout' to ensure that you're not replacing
 *    important data which BIOS used before.
 *
 * ------------------------------------------------------------------------ */
#include <print.h>
#include <x86/io.h>
#include <x86/asm.h>
#include <x86/pg.h>
#include <x86/idt.h>
#include <x86/tss.h>
#include <debug.h>
#include <timer.h>
#include <memory.h>
#include <tskmgr.h>
#include <lock.h>
#include <kbd.h>
#include <inbb.h>
#include <scode.h>

/* 
 * In normal situations, compiler and standard libs did a lot of jobs
 * for us. for example: we can pass parameters to a command line, then
 * read it in the 'main' function. before we enter 'main' function,
 * some invisible works have been done. invoking global constructors is
 * one of them.
 * but we're writing kernel here, nobody do that for us. we have to do
 * it ourselves.
 * also, linker(ld) has an option: --warn-constructors
 * --warn-constructors Warn if any global constructors are used. This is
 *          only useful for a few object file formats. For formats like
 *          COFF or ELF , the linker can not detect the use of global 
 *          constructors.
 * so linker cannot do the job for us too
 */
typedef void (*pfn_global_ctor)(void);
extern  pfn_global_ctor  __init_array_start[];
extern  pfn_global_ctor  __init_array_end[];
#define CTORS_START     (__init_array_start[0])
#define CTORS_END       (__init_array_end)

inline void invoke_global_ctors() {
    pfn_global_ctor *ctor;
    for (void (**ctor)() = &CTORS_START; ctor < CTORS_END; ++ctor) {
        (*ctor)();
    }
}

using namespace lkl;

// -----------------------------------------------------------------------
// Global Variables
lock_t g_screen_lock;
extern inbb_t g_kbd_buffer;
// -----------------------------------------------------------------------


void msg_welcome() {

    color_t col1(color_t::B_GREEN  |
                 color_t::F_YELLOW |
                 color_t::MASK_BLINK);
    color_t col2(color_t::F_YELLOW, color_t::B_GREEN);
    dbg_msg("welcome to ", col2);
    dbg_msg("XOS!\n", col1);
}

void msg_paging_enabled() {
    dbg_msg("Paging Enabled.\n");
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
    x86_asm::set_cr3((uint32_t)addr);

    // enable paging
    x86_asm::turn_paging_on();
    //x86_asm::set_cr0(x86_asm::get_cr0() | 0x80000000);
}

void thread_a(void* arg) {
    
    while(1) {
        g_screen_lock.acquire();
        //dbg_msg("AAAA ");
        g_screen_lock.release();
    }
}

void thread_b(void* arg) {
    
    while(1) {
        g_screen_lock.acquire();
        //dbg_msg("BBBB ");
        g_screen_lock.release();
    }
}

int main() {
    // never move any code before invoke_global_ctors()
    // unless you know what you're doing.
    invoke_global_ctors();

    msg_welcome();
    enable_paging(0x00100000);
    msg_paging_enabled();
    tss_t::init();
    mem_mgr::init();

    auto addr = mem_mgr::alloc(mem_mgr::PT_KERNEL, 1);
    dbg_mhl(
        "kernel pool allocated:",
        (uint32_t)addr,
        color_t::F_LIGHT_GREEN);
    
    uint32_t* ptr = (uint32_t*)addr;

    pic8259a::init();
    interrupt<x86_asm>::init((ig_desc_t*)0x8000, 0x30);
    
    x86_asm::turn_interrupt_on();
    keyboard::init();
    pic8259a::enable(pic8259a::DEV_KEYBOARD);

    task_mgr::init();
    pic8259a::enable(pic8259a::DEV_TIMER);
    
    pit8253::freq(4000);
    task_mgr::begin_thread(thread_a, nullptr, "thA", 5);
    task_mgr::begin_thread(thread_b, nullptr, "thB", 10);

    while(1) {
        // g_screen_lock.acquire();
        // dbg_msg("main ");
        // g_screen_lock.release();
        auto_intr<x86_asm> ai(false);
        scode_t sc = g_kbd_buffer.getc();
        if(sc.is_visible() ||
           sc.is_special()) 
        {
            if(sc.is_key_down()) {
                dbg_char(sc);
            } 
        }
    }
    while(1);
    return 0;
}