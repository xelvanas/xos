#include <thread.h>
#include <x86/idt.h>
#include <debug.h>
#include <memory.h>

ns_lite_kernel_lib_begin
uint32_t       task_mgr::s_states = 0;
queue_t<pcb_t> task_mgr::s_ready_queue;
queue_t<pcb_t> task_mgr::s_all_queue;

void prep_ent_thread(thread_func func, void* arg) {
    x86_asm::turn_interrupt_on();
    func(arg);
}

void task_mgr::init() {

    // already initialized
    if(bit_test(s_states, TMS_INITIALIZED))
        return;

    // order is important, do not reg 'scheduler' before
    // main thread initialized.
    __inner_cur_thrd_as_main_thrd();

    // register scheduler as ISR for 
    interrupt<x86_asm>::reg(
        pic8259a::DEV_TIMER,
        task_mgr::scheduler);
}

void task_mgr::scheduler(uint32_t no) {

    auto cur = current_pcb();

    // ASSERT(s_all_queue.find(&cur->get_alq_node()));

    if(s_ready_queue.empty()) {
        //display();
        x86_asm::turn_interrupt_on();
        return;
    }

    if(cur->get_state() == pcb_t::TS_RUNNING) {
        cur->set_state(pcb_t::TS_READY);
        s_ready_queue.push_back(&cur->get_rdq_node());
    }

    auto node = s_ready_queue.pop_front();
    auto next = node->_pobj;
    next->set_state(pcb_t::TS_RUNNING);
    
    task_switch(cur, next);
}

pcb_t* task_mgr::current_pcb() {
    uint32_t esp = x86_asm::get_esp();
    return (pcb_t*)(esp&0xFFFFF000);
}

void task_mgr::begin_thread(
    thread_func func,
    void* arg,
    const char* name)
{
    if(func == nullptr )
        return;

    auto pcb = (pcb_t*)mem_mgr::alloc(mem_mgr::PT_KERNEL, 1);
    
    uint32_t addr = (uint32_t)pcb;
    dbg_mhl("new pcb:", addr);
    memset((void*)pcb, 0 , pool_t::PAGE_SIZE);
    pcb->set_name(name);
    
    pcb->set_state(pcb_t::TS_READY);
    pcb->set_magic(THREAD_MAGIC);
    pcb->get_rdq_node()._pobj = pcb;
    pcb->get_alq_node()._pobj = pcb;

    addr += 1000;
    addr -= sizeof(intr_stack);
    addr -= sizeof(thread_stack);
    auto thrd_stack = (thread_stack*)(addr);
    pcb->set_kstack((uint32_t*)addr);

    thrd_stack->_arg = (uint32_t)arg;
    thrd_stack->_fun = (uint32_t)func;
    thrd_stack->_eip = (uint32_t)prep_ent_thread; 

    s_ready_queue.push_back(&pcb->get_rdq_node());
    s_all_queue.push_back(&pcb->get_alq_node());
}

void task_mgr::__inner_cur_thrd_as_main_thrd() {
    // main thread initialized
    if(bit_test(s_states, TMS_MAIN_THREAD))
        return;

    auto pcb = current_pcb();
    // it's very dangeous operation,  'esp' can be somewhere 
    // between 'pcb + sizeof(pcb_t)'.  :)
    memset((void*)pcb, 0, sizeof(pcb_t));
    // initialize main thread
    pcb->set_name("main");
    // pcb and kernel stack share same page
    pcb->set_kstack((uint32_t*)((uint32_t)pcb + 0x1000));
    pcb->set_state(pcb_t::TS_RUNNING);
    pcb->set_magic(0xdeaddead);
    pcb->get_rdq_node()._pobj = pcb;
    pcb->get_alq_node()._pobj = pcb;
    //s_ready_queue.push_back(&pcb->get_rdq_node());
    s_all_queue.push_back(&pcb->get_alq_node());
}


ns_lite_kernel_lib_end