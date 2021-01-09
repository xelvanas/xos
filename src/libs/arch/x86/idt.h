#pragma once
#include <x86/io.h>
#include <x86/asm.h>
#include <bit.h>
#include <debug.h>

extern "C" void main_intr_handler(uint16_t num);

#define IDT_DESC_CNT 0x30 // 48

extern void* intr_entry_table[IDT_DESC_CNT];


class pic8259a
{
private:
    static uint32_t _states;
    
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
    bool has_ois() const {
        return lkl::bit_test(_states, STAT_HAS_OIS);
    }

    // use this only after check object has_ois == true
    // otherwise, invalid return value.
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
            isa::enable_intr();
        } else {
            isa::disable_intr();
        }
    }
};

template<typename isa>
class interrupt
{
public:
    static void init_idt(gate_desc_t* idt, uint32_t len) {
        
        for(uint32_t i = 0;i < len; ++i) {
            idt[i].initialize((uint32_t)intr_entry_table[i],
                              0x08,
                              0b01110,
                              0);
        }

        idt_desc_t desc = {
            (uint16_t)(sizeof(gate_desc_t)*len - 1),
            (uint32_t)idt
        };

        x86_asm::load_idt(&desc);
    }
};