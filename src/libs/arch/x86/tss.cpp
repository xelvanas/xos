#include <x86/tss.h>
#include <x86/asm.h>
#include <tskmgr.h>

tss_t g_tss;

uint32_t tss_t::s_states = 0;

void
tss_t::init() {
    if(s_states > 0)
        return;

    g_tss.zeroize();
    g_tss._ss0 = 0x10; // stack segment: second gdt desc
    g_tss._io_bmp_base = sizeof(tss_t);
    g_tss._esp0 = (uint32_t)lkl::task_mgr::current_thread() + 0x1000;

    gdt_desc_t desc;
    x86_asm::store_gdt(&desc);

    uint32_t offset = 0x28;
    // already reserved an entry of gdt at booting stage.
    tss_desc_t& td = *(tss_desc_t*)(desc.address + offset);

    td.reset((uint32_t)&g_tss, sizeof(tss_t)-1);
    td.present(true);
    x86_asm::load_gdt(&desc);
    x86_asm::load_tr(offset);
    s_states = 1;
}