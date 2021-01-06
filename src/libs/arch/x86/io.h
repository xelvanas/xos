#pragma once
#include <stdint.h>

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

private:
    // creating an instance is disallowed.
    x86_isa() {} 
};
