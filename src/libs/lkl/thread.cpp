#include <thread.h>
#include <x86/idt.h>
#include <debug.h>

ns_lite_kernel_lib_begin
uint32_t       task_mgr::s_states = 0;
queue_t<pcb_t> task_mgr::s_ready_queue;
queue_t<pcb_t> task_mgr::s_all_queue;

void task_mgr::init() {

    // already initialized
    if(bit_test(s_states, TMS_INITIALIZED))
        return;

    // register scheduler as ISR for 
    interrupt<x86_asm>::reg(
        pic8259a::DEV_TIMER,
        task_mgr::scheduler);

    __inner_cur_thrd_as_main_thrd();
    // auto pcb = current_pcb();
    // pcb->set_name("main");
    // pcb->set_kstack((uint32_t*)((uint32_t)pcb + 0x1000));
    // pcb->set_state(pcb_t::TS_RUNNING);
    // pcb->set_magic(0x01010101);
    // pcb->get_qnode()._pobj = pcb;
    // s_all_queue.push_back(&pcb->get_qnode());
}

void task_mgr::scheduler(uint32_t no) {
    auto pcb = current_pcb();
    dbg_msg(pcb->get_name());
}

pcb_t* task_mgr::current_pcb() {
    uint32_t esp = x86_asm::get_esp();
    return (pcb_t*)(esp&0xFFFFF000);
}

void task_mgr::__inner_cur_thrd_as_main_thrd() {
    // main thread initialized
    if(bit_test(s_states, TMS_MAIN_THREAD))
        return;

}


ns_lite_kernel_lib_end