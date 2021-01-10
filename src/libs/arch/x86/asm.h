#pragma once
#include <stdint.h>
#include <x86/mmu.h>
#include <bit.h>
#include <string.h>
#include <debug.h>
class x86_asm
{
public:

    // function pointer
    // read 32-bit value
    typedef uint32_t (*fn_rd32_t)();

    // function pointer
    // write 32-bit value
    typedef void (*fn_wt32_t)(uint32_t);


public:
    static inline uint32_t
    get_cr0() {
        uint32_t val;
        asm volatile("mov %%cr0, %0;" : "=r" (val));
        return val;
    }

    static inline void
    set_cr0(uint32_t val) {
        asm volatile("mov %0, %%cr0" : : "r" (val));
    }

    static inline uint32_t
    get_cr3() {
        uint32_t val;
        asm volatile("mov %%cr3, %0;" : "=r" (val));
        return val;
    }

    static inline void
    set_cr3(uint32_t val) {
        asm volatile("mov %0, %%cr3" : : "r" (val));
    }

    static inline void
    load_gdt(const gdt_desc_t *gdt) {
        asm volatile("lgdt %0"::"m" (*gdt));
    }

    static inline void
    load_idt(const idt_desc_t *idt) {
        asm volatile("lidt %0"::"m" (*idt));
    }

    static inline void
    store_gdt(gdt_desc_t *gdt) {
        asm volatile("sgdt %0":"=m" (*gdt));
    }

    static inline void
    store_idt(idt_desc_t *idt) {
        asm volatile("sidt %0":"=m" (*idt));
    }

    static inline uint32_t
    get_esp() {
        uint32_t esp;
        asm ("mov %%esp, %0" : "=g" (esp));
        return esp;
    }

    static inline uint32_t
    get_eflags() {
        uint32_t eflags;
        asm volatile("pushfl; popl %0" : "=g"(eflags));
        return eflags;
    }

    static inline void
    turn_interrupt_on() {
        asm volatile("sti");
    }

    static inline void
    turn_interrupt_off() {
        asm volatile("cli" : : : "memory");
    }

    enum
    {
        CR0_PG   = 0x8000'0000, // Paging (bit 31)
        CR0_CD   = 0x4000'0000, // Cache Disable (bit 30)
        CR0_NW   = 0x2000'0000, // Not Write Through (bit 29)
        CR0_AM   = 0x0002'0000, // Alignment Mask (bit 18)
        CR0_WP   = 0x0000'8000, // Write Protect (bit 16)
        CR0_NE   = 0x0000'0020, // Numeric Error (bit 5)
        CR0_ET   = 0x0000'0010, // Extension Type (bit 4)
        CR0_EM   = 0x0000'0004, // Emulation (bit 2)
        CR0_MP   = 0x0000'0002, // Monitor Coprocessor (bit 1)
        CR0_PE   = 0x0000'0001, // Protection Enable (bit 0)
    };

    static inline bool
    is_protected_mode() {
        return lkl::bit_test(x86_asm::get_cr0(), CR0_PE);
    }

    static inline void
    turn_protected_mode_on() {
        __inner_set_reg_state(get_cr0, set_cr0, true, CR0_PE);
    }

    static inline void
    turn_protected_mode_off() {
        __inner_set_reg_state(get_cr0, set_cr0, false, CR0_PE);
    }

    // move gdt to new address
    // return: where gdt ends
    static inline void*
    move_gdt_to(void* dst, size_t len) {
        
        if(dst == nullptr) {
            return nullptr;
        }
        
        gdt_desc_t desc;
        store_gdt(&desc);

        if(desc._size + 1 > len) {
            // buffer is too small
            return nullptr;
        }
        /* it's not necessary to turn off 'protected mode' to move
         * 'gdt'. actually, I tried to turn it off, but it didn't 
         * work.  (:0)
         * fortunately, I don't have to do that. but should be careful
         * of that if the content of gdt entry changed and you mess it
         * up, no bad thing will happen immediately. each segment 
         * register has a 'visible' portion and an 'invisible' portion.
         * for example: code segment. to change its 'visible' portion
         * needs a far 'jmp' or 'call'. then the processor will load
         * base+limit into its 'invisible' portion.
         * 
         * the problem is that we cannot realize we mess gdt up until
         * far 'jmp/call' occurs. (which made me crazy for hours.)
         */ 

        // turn_protected_mode_off();
        // if(is_protected_mode()) {
        //     // cannot turn of 'protected mode'
        //     dbg_msg("cannot turn 'protected mode' off!\n");
        //     return nullptr;
        // }

        if(memcpy_s(
            dst,
            len,
            (void*)desc._address,
            desc._size+1) == 0) {
            desc._address = (uint32_t)dst;
            load_gdt(&desc);
            return (uint8_t*)dst + desc._size + 1;
        }
        return nullptr;
    }

    static inline bool
    is_paging_on() {
        return lkl::bit_test(x86_asm::get_cr0(), CR0_PG);        
    }

    static inline void
    turn_paging_on() {
        __inner_set_reg_state(get_cr0, set_cr0, true, CR0_PG);
    }

    static inline void
    turn_paging_off() {
        __inner_set_reg_state(get_cr0, set_cr0, false, CR0_PG);
    }

private:

    template<typename fnRD, typename fnWT>
    static inline void
    __inner_set_reg_state(fnRD rd, fnWT wt, bool ns, uint32_t flag) {
        // old value
        uint32_t ov = rd();
        bool os = lkl::bit_test(ov, flag);
        // set cr0 if old state != new state
        if(os != ns) {
            // set flag
            lkl::bit_set(ov, CR0_PG, ns);
            // write flag
            wt(ov);
        }
    }
    
    /*
     * Abandoned: using function points may cause
     * 'inline directives' cease to be effective.
     *
    // common function for read/write register state
    static inline void
    __inner_set_reg_state(fn_rd32_t rd,
                          fn_wt32_t wt,
                          bool ns,
                          uint32_t flag) 
    {
        // old value
        uint32_t ov = rd();
        bool os = lkl::bit_test(ov, flag);
        // set cr0 if old state != new state
        if(os != ns) {
            // set flag
            lkl::bit_set(ov, CR0_PG, ns);

            // write flag
            wt(ov);
        }
    }
    */

    // creating an instance is disallowed.
    x86_asm() {} 
};