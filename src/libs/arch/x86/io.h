#pragma once
#include <stdint.h>
#include <x86/mmu.h>

class x86_isa
{
public:

    // read a byte from port
    static inline
    uint8_t inb(uint16_t port) {
        uint8_t data;
        asm volatile("in %1, %0" : "=a" (data) : "d" (port));
        return data;
    }

    // read 16-bit data from port
    static inline uint16_t
    inw (uint16_t port) {
        uint16_t data;
        asm volatile ("inw %w1, %w0" : "=a" (data) : "Nd" (port));
        return data;
    }

    // read 32-bit data from port
    static inline uint32_t
    inl (uint16_t port) {
        uint32_t data;
        asm volatile ("inl %w1, %0" : "=a" (data) : "Nd" (port));
        return data;
    }

    // read 'cnt'(max:0xff) bytes from port
    static inline void
    insb (uint16_t port, void *addr, uint32_t cnt) {
        asm volatile (
            "rep insb" :
            "+D" (addr), "+c" (cnt) :
            "d" (port) : "memory");
    }

    // read 'cnt'(max:0xffff) 16-bit data from port
    static inline void 
    insw(uint16_t port, void* addr, uint32_t cnt) {
        asm volatile(
            "cld;" "rep insw" :
            "+D" (addr), "+c" (cnt) :
            "d" (port) :
            "memory", "cc");  
    }

    // read 'cnt'(max:0xffffffff) 32-bit data from port
    static inline void
    insl (uint16_t port, void *addr, uint32_t cnt) {
        asm volatile (
            "rep insl" :
            "+D" (addr), "+c" (cnt) :
            "d" (port) : "memory");
    }

    //write a byte to port
    static inline void 
    outb(uint16_t port, uint8_t data) {
        asm volatile(
            "out %0, %1" :
            : 
            "a" (data), "d" (port));
    }

    //write 16-bit data to port
    static inline void 
    outw(uint16_t port, uint16_t data) {
        asm volatile(
            "out %0, %1" :
            : 
            "a" (data), "d" (port));
    }

    //write 32-bit data to port
    static inline void
    outl (uint16_t port, uint32_t data) {
        asm volatile ("outl %0, %w1" : : "a" (data), "Nd" (port));
    }

    //write 'cnt'(max:0xff) bytes to port
    static inline void
    outsb (uint16_t port, const void *addr, uint32_t cnt) {
        asm volatile ("rep outsb" : "+S" (addr), "+c" (cnt) : "d" (port));
    }

    //write 'cnt'(max:0xffff) 16-bit data to port
    inline void 
    outsw(uint16_t port, const void* addr, uint32_t cnt) {
        asm volatile(
            "cld; rep outsw" :
            "+S" (addr), "+c" (cnt) :
            "d" (port):
            "cc");
    }

    //write 'cnt'(max:0xffffffff) 32-bit data to port
    static inline void
    outsl (uint16_t port, const void *addr, uint32_t cnt) {
        asm volatile (
            "rep outsl" :
            "+S" (addr), "+c" (cnt) :
            "d" (port));
    }

    
    static inline uint32_t
    get_cr0()
    {
        uint32_t val;
        asm volatile("mov %%cr0, %0;" : "=r" (val));
        return val;
    }

    static inline void
    set_cr0(uint32_t val)
    {
        asm volatile("mov %0, %%cr0" : : "r" (val));
    }

    static inline uint32_t
    get_cr3()
    {
        uint32_t val;
        asm volatile("mov %%cr3, %0;" : "=r" (val));
        return val;
    }

    static inline void
    set_cr3(uint32_t val)
    {
        asm volatile("mov %0, %%cr3" : : "r" (val));
    }

    static inline void
    load_gdt(const gdt_desc_t *gdt)
    {
        asm volatile("lgdt %0"::"m" (*gdt));
    }

    static inline void
    load_idt(const idt_desc_t *idt)
    {
        asm volatile("lidt %0"::"m" (*idt));
    }

    static inline void
    store_gdt(gdt_desc_t *gdt)
    {
        asm volatile("sgdt %0":"=m" (*gdt));
    }

    static inline void
    store_idt(idt_desc_t *idt)
    {
        asm volatile("sidt %0":"=m" (*idt));
    }

    static inline uint32_t
    get_esp()
    {
        uint32_t esp;
        asm ("mov %%esp, %0" : "=g" (esp));
        return esp;
    }

private:
    // creating an instance is disallowed.
    x86_isa() {} 
};
