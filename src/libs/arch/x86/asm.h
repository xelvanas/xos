#pragma once
#include <stdint.h>
#include <x86/mmu.h>

class x86_asm
{
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
    enable_intr() {
        asm volatile("sti");
    }

    static inline void
    disable_intr() {
        asm volatile("cli" : : : "memory");
    }

private:
    // creating an instance is disallowed.
    x86_asm() {} 
};