#pragma once
#include <lkl.h>
#include <x86/pg.h>
#include <bitmap.h>
#include <pool.h>
#include <string.h>
#include <lock.h>

/*
 * Real Mode Address Space (less than 1 MByte)
 *
 * when a typical x86 PC boots it will be in 'Real Mode', with an active
 * BIOS. during the time the CPU remains in 'Real Mode', IRQ0 (the clock)
 * will fire repeatedly, and the hardware that is used to boot the PC
 * (floppy, hard disk, CD, Network Card, USB) will also generate IRQs.
 * this means that during the PC boot process, the 'Real Mode' IVT must
 * be carefully preserved, because it is being used.
 * 
 * when the IVT is activated by an IRQ, it will call a BIOS routine to
 * handle the IRQ. Bootloaders will also access BIOS functions. this
 * means that the two memory workspace that the BIOS uses (the BDA and
 * the EBDA) must also be carefully preserved during boot. also, every
 * time the BIOS handles an IRQ0 (18 times a second), several bytes in
 * the BDA get overwritten by the BIOS - so do not attempt to store
 * anything there while IRQs are active in 'Real Mode'.
 * 
 * after the BIOS functions have been called, and kernel is loaded memory
 * somewhere, the bootloader or kernel may exit 'Real Mode' forever
 * (often by going into 32-bit Protected Mode). if the kernel never uses
 * 'Real Mode' again, then the first 0x500 bytes of memory in the PC may
 * be reused and overwritten. (However, it is very common to temporarily
 * return to 'Real Mode" in order to change the 'Video Display Mode'.)
 * 
 * when the CPU is in 'Protected Mode', System Management Mode (SMM) is
 * still invisibly active, and cannot be shut off. SMM also seems to use
 * the EBDA. so the EBDA memory area should never be overwritten.
 * 
 * Note: the EBDA is a variable-sized memory area (on different BIOSes).
 * if it exists, it is always immediately below 0xA0000 in memory. it is
 * absolutely guranteed to be at most 128 KByte in size. Older computers
 * typically uses 1KByte from 0x9FC00 - 0x9FFFF, modern firmware can be
 * found using significantly more. you can determine the size of the EBDA
 * by using BIOS function 'INT 12h', or by examing the word at 0x413 in
 * the BDA. both of these methods will tell you how much conventional
 * memory is usable before the EBDA.
 * 
 * it should also be noted that bootloader code is loaded and running in
 * the memory at physical address 0x7c00 through 0x7DFF. so that memory
 * area is likely to also be unusable until execution has been
 * transferred to a second stage bootloader, or to kernel.
 * 
 * ┌──────────┬──────────┬──────┬─────────────────┬────────────────────┐
 * │  start   │  end     │ size │  description    │       type         │
 * ├──────────┼──────────┼──────┼─────────────────┼─────────┬──────────┤
 * │0x00000000│0x000003ff│1KB   │Real Mode IVT    │unusable │          │
 * ├──────────┼──────────┼──────┼─────────────────┤in Real  │          │
 * │0x00000400│0x000004ff│256B  │BDA              │Mode     │ 640KB    │
 * ├──────────┼──────────┼──────┼─────────────────┼─────────┤ RAM      │
 * │0x00000500│0x00007bff│≈30KB │Conventional Mem │usable   │ Low      │
 * ├──────────┼──────────┼──────┼─────────────────┤memory   │ Memory   │
 * │0x00007c00│0x00007dff│1KB   │OS BootSector    │         │          │
 * ├──────────┼──────────┼──────┼─────────────────┤         │          │
 * │0x00007e00│0x0007ffff│≈480KB│Conventional Mem │         │          │
 * ├──────────┼──────────┼──────┼─────────────────┼─────────┤          │
 * │0x00080000│0x0009ffff│128KB │EBDA             │unusable │          │
 * ├──────────┼──────────┼──────┼─────────────────┼─────────┼──────────┤
 * │0x000a0000│0x000bffff│128KB │Video Display Mem│unusable │          │
 * ├──────────┼──────────┼──────┼─────────────────┼─────────┤384 KB    │
 * │0x000c0000│0x000c7fff│32KB  │Video BIOS       │unusable │System/   │
 * ├──────────┼──────────┼──────┼─────────────────┼─────────┤Reserved. │
 * │0x000c8000│0x000effff│160KB │BIOS Expansions  │unusable │Upper     │
 * ├──────────┼──────────┼──────┼─────────────────┼─────────┤Memory    │
 * │0x000f0000│0x000fffff│64KB  │Motherboard BIOS │unusable │          │
 * └──────────┴──────────┴──────┴─────────────────┴─────────┴──────────┘
 * 
 */

/*
 * 'LOADER' has been loaded at 0x00300000. there're many usable space
 * in first 1MB memory. at this stage, 'OS boot sector'(0x7c00-0x7e00)
 * can be replaced safely, then the OS will crash. :) why is that? 
 * because GDT was there. that's why the 'move_gdt_to(0x500)' appeared
 * at beginning of 'loader::main()'. I decided to leave 0x0500-0x0600 to
 * GDT in case of any extension might be made later.
 * 
 * The default address of 'stack' was 0xffff, we don't have 'EBDA' here,
 * 'stack' was moved to 0x9F000. since 'stack' grows downward (highest
 * address to lowest) and 'BIOS e8200' memory detected that first free
 * memory segment is: 0x00000000-0x0009FC00. 
 * 
 * typical output by a call to 'INT 0x15, EAX=0xE820' in bochs:
 * Base Address       | Length             | Type
 * 0x0000000000000000 | 0x000000000009FC00 | Free Memory (1)
 * 0x000000000009FC00 | 0x0000000000000400 | Reserved Memory (2)
 * 0x00000000000E8000 | 0x0000000000018000 | Reserved Memory (2)
 * 0x0000000000100000 | 0x0000000001F00000 | Free Memory (1)
 * 0x00000000FFFC0000 | 0x0000000000040000 | Reserved Memory (2)
 *  
 * there's a little bit space between 0x9F000-0x9FC00 available. the
 * 'Memory Detecting' results saved there.
 * 
 * [ABANDONED
 * - 0x9F000 - 0x9F003:  number of ARDS
 * - 0x9F004 - 0x9FC00:  ARDS data (sizeof(ARDS) is 20Byte) ABANDONED]
 * there was a little segment (0x9F000-0x9FC00) when bochs.megs = 32,
 * but it disappeared since I changed bochs.megs = 64. so, I have to 
 * move ards to 0x800 - higher place.
 * 
 * 
 * 'LOADER' has 48 Interrupt Descriptors, each one is 8byte long. it took 
 * 48*8 = 384 byte. 0x0600-0x0800 would be enough to store IDT.
 * 
 * Here is what we used for now
 *  address     property
 * ┌────────────────┬───────┬───────────────────────────────────────────┐
 * │  0x500-0x600   │ 0x100 │  GDT                                      │
 * ├────────────────┼───────┼───────────────────────────────────────────┤
 * │  0x600-0x800   │ 0x200 │  IVT                                      │
 * ├────────────────┼───────┼───────────────────────────────────────────┤
 * │  0x9F000-Lower │ ...   │  Stack                                    │
 * ├────────────────┼───────┼───────────────────────────────────────────┤
 * │  0x0800-Higher │ ...   │  ARDS (can be replaced safely)            │
 * ├────────────────┼───────┼───────────────────────────────────────────┤
 * │                │       │                                           │
 * └────────────────┴───────┴───────────────────────────────────────────┘
 * Interrupt Handlers might be put in first 1MB memory later.
 * 
 */

ns_lite_kernel_lib_begin

class mem_mgr
{
private:
    static pool_t _kp_pool;
    static pool_t _kv_pool;
    static lock_t _lock;
private:
    enum
    {
        KER_P_BMP_BUF    = 0x0800,
        KER_V_BMP_BUF    = 0x1800,
        USR_P_BMP_BUF    = 0x2800,
        USR_V_BMP_BUF    = 0x3800,
        KER_V_ADDR_START = 0xC000'0000, // kernel starting virtual addr
        USR_V_ADDR_START = 0x0100'0000, // user starting virtual addr
        PTE_BOUNDARY     = 0x0040'0000, // each PTE maps 4MB
        PTE_RANGE_MASK   = ~(0x0040'0000-1),
    };
public:

    enum page_type_t
    {
        PT_KERNEL   = 0,
        PT_USER     = 1,
    };

    static void
    init();

    /*
     * 'alloc' allocates physical pages and virtual pages
     * maps virtual pages on phsical pages if allocation successed.
     * return 'nullptr' if failed.
     */
    static void*
    alloc(page_type_t pt, uint32_t cnt);

    static void*
    alloc_phys_page(uint32_t cnt);

    static uint32_t
    v2p(uint32_t addr);

private:
    // creating an instance is disallowed.
    mem_mgr() = delete;

    static inline uint32_t
    __inner_detect_unallocated_pte(
        uint32_t vbeg,
        uint32_t vend);

    static inline pde_t*
    __inner_get_pde_v(uint32_t vaddr) {
        // 1. point twice PDE start
        // 2. add PDE offset * 4 (each index occupies 4 bytes)
        return (pde_t*)((0xfffff000) + 
        (pde_t::get_pde_index(vaddr)<<2));
    }

    static inline pte_t*
    __inner_get_pte_v(uint32_t vaddr) {
        return (pte_t*)(0xffc00000 + // point to PDE:1023
        ((vaddr & 0xffc00000) >> 10) + // PDE as PTE (to locate PDE)
        (pte_t::get_pte_index(vaddr) << 2)); // real PTE offset
    }

    static inline void
    __inner_map_virtual_on_phys(
        uint32_t vaddr,
        uint32_t paddr);

    static void*
    __inner_alloc_pages(
        pool_t&  mpool,
        pool_t&  vpool,
        uint32_t cnt);

    // len = 0 if failed
    static void
    __inner_detect_valid_mem(
        uint32_t& addr,
        uint32_t& len);
};


ns_lite_kernel_lib_end