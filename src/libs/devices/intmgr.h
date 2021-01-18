#pragma once
#include <x86/io.h>
#include <x86/asm.h>
#include <bit.h>
#include <debug.h>

inline extern const int
SYSTEM_IRQ_COUNT = 0x30;

typedef void (*irq_handler)();

class intr_guard
{
    bool _old_intr     : 1 = false;
public:
    enum
    {
        STAT_HAS_OIS        = 0x0002, // has old interrupt state
        STAT_OIS            = 0x0004, // old interrupt state
        EFLAGS_IF           = 0x0200  //eflags.IF
    };

    intr_guard(bool intr) 
        : _old_intr(x86_asm::is_interrupt_on())
    {
        set_intr(intr);
    }

    ~intr_guard() 
    {
        set_intr(_old_intr);
    }

private:
    void set_intr(bool st) {
        if (st == true) {
            x86_asm::turn_interrupt_on();
        } else {
            x86_asm::turn_interrupt_off();
        }
    }
};

class intr_mgr
{
    bool _initialized : 1 = false;
private:
    irq_handler _irq_handlers[SYSTEM_IRQ_COUNT];
private:
    intr_mgr() = default;
public:
    intr_mgr(intr_mgr&&)            = delete;
    intr_mgr(const intr_mgr&)       = delete;
    void operator=(intr_mgr&&)      = delete;
    void operator=(const intr_mgr&) = delete;
public:
    enum
    {
        IRQ_NAME_DE     = 0x00, // divided-by-zero error
        IRQ_NAME_DBG    = 0x01, // debug
        IRQ_NAME_NMI    = 0x02, // non-maskable interrupt 
        IRQ_NAME_BP     = 0x03, // breakpoint
        IRQ_NAME_OF     = 0x04, // overflow
        IRQ_NAME_BR     = 0x05, // bound range exceeded
        IRQ_NAME_UD     = 0x06, // undefined opcode
        IRQ_NAME_NM     = 0x07, // device not available
        IRQ_NAME_DF     = 0x08, // double fault
        IRQ_NAME_TS     = 0x0A, // invalid tss
        IRQ_NAME_NP     = 0x0B, // segment not present
        IRQ_NAME_SS     = 0x0C, // stack-segment fault
        IRQ_NAME_GP     = 0x0D, // general protection fault
        IRQ_NAME_PF     = 0x0E, // page fault
        IRQ_NAME_MF     = 0x10, // x87 floating-point exception
        IRQ_NAME_AC     = 0x11, // alignment check
        IRQ_NAME_MC     = 0x12, // machine check
        IRQ_NAME_XM     = 0x13, // SIMD floating-point exception
        IRQ_NAME_VE     = 0x14, // virtualization exception
        IRQ_NAME_SX     = 0x1E, // security exception
        IRQ_NAME_TIMER  = 0x20, // timer
        IRQ_NAME_KBD    = 0x21, // keyboard
        IRQ_NAME_CSCD   = 0x22, // cascade
        IRQ_NAME_COM2   = 0x23, // COM2
        IRQ_NAME_COM1   = 0x24, // COM1
        IRQ_NAME_LPT2   = 0x25, // Parallel port 2 (LPT2)
        IRQ_NAME_FLOPPY = 0x26, // Floppy Disk
        IRQ_NAME_LPT1   = 0x27, // parallel port 1 (LPT1)
        IRQ_NAME_RTIMER = 0x28, // CMOS real time clock
        IRQ_NAME_PS2    = 0x2C, // ps/2
        IRQ_NAME_FE     = 0x2D, // fpu exception
        IRQ_NAME_HDD1   = 0x2E, // primary ATA HDD
        IRQ_NAME_HDD2   = 0x2F, // secondary ATA HDD
    };
public:
    bool
    init(void* idt_addr, uint32_t size) 
    {
        if(_initialized)
            return _initialized;
        
        if(size < sizeof(ig_desc_t)*SYSTEM_IRQ_COUNT)
            return false;

        const uint16_t GDT_CODE_SELECTOR = 0x08; // always same value

        ig_desc_t* idesc = (ig_desc_t*)idt_addr;
        
        for(uint32_t i = 0; i < SYSTEM_IRQ_COUNT; ++i) {
            idesc[i].reset(
                GDT_CODE_SELECTOR,
                (uint32_t)_irq_handlers[i]);
            idesc[i].present(true);
        }

        idt_desc_t tdesc = {
            (uint16_t)(sizeof(ig_desc_t)*SYSTEM_IRQ_COUNT - 1),
            (uint32_t)idt_addr
        };
        x86_asm::load_idt(&tdesc);
        _initialized = true;
        return true;
    }

    void
    reg(uint32_t no, irq_handler handler) {
        ASSERT(no < SYSTEM_IRQ_COUNT && handler != nullptr);
        intr_guard guard(false);
        _irq_handlers[no] = handler;
    }

    static const char*
    irq_to_name(uint32_t vct);

    static intr_mgr&
    instance() {
        static intr_mgr s_intr_mgr;
        return s_intr_mgr;
    }
};
