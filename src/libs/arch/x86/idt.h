#pragma once
#include <x86/io.h>
#include <x86/asm.h>
#include <bit.h>
#include <debug.h>

#define IRQ_CNT 0x30

extern void* isr_tbl[IRQ_CNT];

extern "C" void main_cxx_isr(uint32_t num);

class pic8259a
{
private:
    static uint32_t    _states;
    enum
    {
        PIC_INITIALIZED = 0x00000001,
        PIC_M_CTRL      = 0x20,
        PIC_M_DATA      = 0X21,
        PIC_S_CTRL      = 0xA0,
        PIC_S_DATA      = 0xA1
    };   
public:

/* 
 * INTERRUPT VECTORs:
 * 0x20: timer
 * 0x21: keyboard
 * 0x22: cascade
 * 0x23: serial port 2
 * 0x24: serial port 1
 * 0x25: parallel port 2
 * 0x26: floppy
 * 0x27: parallel port 1
 * 0x28: real time clock
 * 0x29: redirect
 * 0x2A: reserved
 * 0x2B: reserved
 * 0x2C: ps/2
 * 0x2D: fpu exception
 * 0x2E: hard disk
 * 0x2F: reserved
 */
    enum DevIntrVct
    {
        DIV_BASE        = 0x20,
        DEV_TIMER       = 0x20, 
        DEV_KEYBOARD    = 0x21, 
        DEV_CASCADE     = 0x22, 
        DEV_SERI_PORT2  = 0x23, 
        DEV_SERI_PORT1  = 0x24, 
        DEV_PARL_PORT2  = 0x25, 
        DEV_FLOPPY      = 0x26, 
        DEV_PARL_PORT1  = 0x27, 
        DEV_RTC         = 0x28, 
        DEV_REDIRECT    = 0x29, 
        DEV_PS2         = 0x2C, 
        DEV_FPU_EXCEPT  = 0x2D, 
        DEV_HDD         = 0x2E,
    };
    static void init();

    static bool initialized() {
        return lkl::bit_test(_states, PIC_INITIALIZED);
    }
    
    static void enable(DevIntrVct div) {
        __inner_set_div(div, false);
    }

    static void disable(DevIntrVct div) {
        __inner_set_div(div, true); 
    }

    static uint8_t get_master_imr() {
        return x86_io::inb(PIC_M_DATA);
    }

    static uint8_t get_slave_imr() {
        return x86_io::inb(PIC_S_DATA);
    }

private:
    static void
    __inner_set_div(DevIntrVct div, bool b) {
        uint8_t dev = div-DIV_BASE;
        uint8_t imr = dev < 8 ? get_master_imr() : get_slave_imr();
        lkl::bit_set(imr, 1 << dev, b);
        x86_io::outb(
            dev < 8 ? PIC_M_DATA: PIC_S_DATA,
            imr);
    }
};

template<typename isa>
class auto_intr
{
    uint8_t _states;
public:
    enum
    {
        STAT_HAS_OIS        = 0x0002, // has old interrupt state
        STAT_OIS            = 0x0004, // old interrupt state
        EFLAGS_IF           = 0x0200  //eflags.IF
    };

    auto_intr(bool intr) : _states(0) {
        change_intr_once(intr);
    }

    ~auto_intr() 
    {
        if (has_ois()) {
            bool saved = get_ois();
            if (saved != current_intr_state()) {
                // restore interrupt state
                set_intr(saved);
            }
        }
    }

private:
    // ois: old interrupt state
    bool has_ois() const {
        return lkl::bit_test(_states, STAT_HAS_OIS);
    }

    // use this only after check object has_ois == true
    // otherwise, invalid value.
    bool get_ois() const {
        return lkl::bit_test(_states, STAT_OIS);
    }

    static bool
    current_intr_state() {
        //return isa::current_intr_state();
        return lkl::bit_test(isa::get_eflags(), EFLAGS_IF);
    }

    // cannot change intr twice
    void change_intr_once(bool st) {
        if (has_ois() == false) 
        {
            bool old = current_intr_state();
            lkl::bit_set(_states, STAT_HAS_OIS, true);
            lkl::bit_set(_states, STAT_OIS, old);
            if (old != st) {
                set_intr(st);
            }
        }
    }

    // unconditional set
    void set_intr(bool st) {
        if (st == true) {
            isa::turn_interrupt_on();
        } else {
            isa::turn_interrupt_off();
        }
    }
};

template<typename isa>
class interrupt
{
public:
    typedef void (*isr_t)(uint32_t no);
    enum 
    {
        // idt entry count 
        IDT_ENT_CNT  = IRQ_CNT,
        GDT_CODE_SEG = 0x08,
        IDT_TYPE     = 0b01110
    };

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
    static isr_t s_isrs[IDT_ENT_CNT];
public:
    static void init(gate_desc_t* ent, uint32_t len) {
        
        for(uint32_t i = 0;i < len; ++i) {
            ent[i].initialize((uint32_t)isr_tbl[i],
                              GDT_CODE_SEG,
                              IDT_TYPE,
                              0);
        }

        idt_desc_t desc = {
            (uint16_t)(sizeof(gate_desc_t)*len - 1),
            (uint32_t)ent
        };
        x86_asm::load_idt(&desc);
    }

    static bool
    reg(uint32_t vct, isr_t han) {
        if(vct < IDT_ENT_CNT) {
            s_isrs[vct] = han;
            return true;
        }
        return true;
    }

    static void
    display_name(uint32_t vct) 
    {
        dbg_mhl("IRQ: ", vct, 0x0E);

        switch(vct)
        {
            case IRQ_NAME_DE:
                 dbg_msg("#DE: divided-by-zero error\n");
                 break;
            case IRQ_NAME_DBG:
                 dbg_msg("#DB: divided-by-zero error\n");
                 break;
            case IRQ_NAME_NMI:
                 dbg_msg("#NMI: non-maskable interrupt\n");
                 break;
            case IRQ_NAME_BP:
                 dbg_msg("#BP breakpoint\n");
                 break;
            case IRQ_NAME_OF:
                 dbg_msg("#OF overflow\n");
                 break;
            case IRQ_NAME_BR:
                 dbg_msg("#BR bound range exceeded\n");
                 break;
            case IRQ_NAME_UD:
                 dbg_msg("#UD undefined opcode\n");
                 break;
            case IRQ_NAME_NM:
                 dbg_msg("#NM device not available\n");
                 break;
            case IRQ_NAME_DF:
                 dbg_msg("#DF double fault\n");
                 break;
            case IRQ_NAME_TS:
                 dbg_msg("#TS invlaid TSS\n");
                 break;
            case IRQ_NAME_NP:
                 dbg_msg("#NP segment not present\n");
                 break;
            case IRQ_NAME_SS:
                 dbg_msg("#SS stack-segment fault\n");
                 break;
            case IRQ_NAME_GP:
                 dbg_msg("#GP general protection fault\n");
                 break;
            case IRQ_NAME_PF:
                 dbg_msg("#PF page fault\n");
                 break;
            case IRQ_NAME_MF:
                 dbg_msg("#MF x87 floating-point exception\n");
                 break;
            case IRQ_NAME_AC:
                 dbg_msg("#AC alignment check\n");
                 break;
            case IRQ_NAME_MC:
                 dbg_msg("#MC machine check\n");
                 break;
            case IRQ_NAME_XM:
                 dbg_msg("#XM SIMD floating-point exception\n");
                 break;
            case IRQ_NAME_VE:
                 dbg_msg("#VE virtualization exception\n");
                 break;
            case IRQ_NAME_SX:
                 dbg_msg("#SX security exception\n");
                 break;
            case IRQ_NAME_TIMER:
                 dbg_msg("Peripheral: Timer\n");
                 break;
            case IRQ_NAME_KBD:
                 dbg_msg("Peripheral: Keyboard\n");
                 break;
            case IRQ_NAME_CSCD:
                 dbg_msg("Peripheral: cascade PIC\n");
                 break;
            case IRQ_NAME_COM2:
                 dbg_msg("Peripheral: COM2\n");
                 break;
            case IRQ_NAME_COM1:
                 dbg_msg("Peripheral: COM1\n");
                 break;
            case IRQ_NAME_LPT2:
                 dbg_msg("Peripheral: Parallel Port 2\n");
                 break;
            case IRQ_NAME_FLOPPY:
                 dbg_msg("Peripheral: Floppy\n");
                 break;
            case IRQ_NAME_LPT1:
                 dbg_msg("Peripheral: Parallel Port 1\n");
                 break;
            case IRQ_NAME_PS2:
                 dbg_msg("Peripheral: PS/2 Mouse\n");
                 break;
            case IRQ_NAME_FE:
                 dbg_msg("Peripheral: FPU exception\n");
                 break;
            case IRQ_NAME_HDD1:
                 dbg_msg("Peripheral: HDD1\n");
                 break;
            case IRQ_NAME_HDD2:
                 dbg_msg("Peripheral: HHD2\n");
                 break;
            default:
                 dbg_mhl("Unknown Number: ", vct, 0x06);
        }
    }
};

template<typename T>
interrupt<T>::isr_t
interrupt<T>::s_isrs[interrupt<T>::IDT_ENT_CNT];
