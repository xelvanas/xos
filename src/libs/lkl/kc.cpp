#include <kc.h>
#include <x86/pg.h>
#include <x86/asm.h>
#include <print.h>
#include <debug.h>


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

ns_lite_kernel_lib_begin

void
k_creator::invoke_global_ctors() {
    pfn_global_ctor *ctor;
    for (void (**ctor)() = &CTORS_START; ctor < CTORS_END; ++ctor) {
        (*ctor)();
    }
}

void
k_creator::turn_on_paging() {
    // pge_t pge;
    // pge.writable(true);
    // pg_dir_t dir((pge_t*)addr, PD_ENT_NUM);
    // dir.fill(0, PD_ENT_NUM, pge, 0);

    // // fill first PDE
    // pge.present(true);
    // pge.address((uint32_t)addr + PAGE_SIZE); // first PTE address
    // dir[0] = pge;

    // // last PDE
    // pge.address((uint32_t)addr);
    // dir[PD_ENT_NUM-1] = pge;

    // // fill first page table
    // dir.reset((pge_t*)((uint32_t)addr + PAGE_SIZE), PT_ENT_NUM);
    // pge.address(0); // 0x00000000 -> 0x00400000;
    // dir.fill(0, PT_ENT_NUM, pge);

    // x86_asm::set_cr3((uint32_t)addr);
    // x86_asm::turn_paging_on();
}

void
k_creator::init_tss() {
    //

}

bool
k_creator::initialize() {
    // never move any code before invoke_global_ctors()
    // unless you know what you're doing.
    invoke_global_ctors();
    __inner_show_welcome();
    x86_asm::move_gdt_to((void*)LML_GDT_BASE, LML_GDT_SIZE);


    _initialized = true;
    return _initialized;
}


void
k_creator::__inner_show_welcome() {
    color_t col1(color_t::B_GREEN |
            color_t::F_YELLOW     |
            color_t::MASK_BLINK);
    color_t col2(color_t::F_YELLOW, color_t::B_GREEN);
    dbg_msg("welcome to ", col2);
    dbg_msg("XOS!\n", col1);
}






















ns_lite_kernel_lib_end