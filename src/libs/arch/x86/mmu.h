#pragma once
#include <stdint.h>

/* ---------------------------------------------------------------------------
 * GDT and Segment Registers
 * GDT and segmentations are originally created to isolate processes and 
 * the data used by those processes. it's not the only way to do isolate
 * processes.
 * 
 * Nowadays, most modern operating systems use paging to isolate 
 * processes. unfortunately Intel's x86 CPUS force us to use a GDT in
 * protected mode, CPU uses segment registers and GDT entries to address
 * 4 gigabytes (4GB) memory.
 * 
 * In Protected Mode, general registers are extended to 32 bits, segment
 * registers no longer be used like in 16-bit real mode each of segment 
 * registers cs, ds, ss, es, fs and gs stores so-called segment selector.
 * 
 * cs (code segment)
 *  15                          3  2  1   0
 * ┌─────────────────────────────┬───┬─────┐
 * │       Index of GDT/LDT      │ T │ CPL │
 * └─────────────────────────────┴───┴─────┘
 * ds (data segment)
 *  15                          3  2  1   0
 * ┌─────────────────────────────┬───┬─────┐
 * │       Index of GDT/LDT      │ T │ RPL │
 * └─────────────────────────────┴───┴─────┘
 * ss, es, fs and gs are same as 'ds'.
 * 
 * 0-1 bits: 
 *   CPL: (current privilege level), only stored in 'cs', the highest
 *        privilege level 0, level 0 is commonly known as Kernel Mode for
 *        Linux and Ring 0 for Windows-based operating systems.
 *   RPL: (request privilege level): if RPL is not privileged enough to 
 *        access a high privileged segment an error will occur.
 *   T:   type of descriptor, denotes descriptor position in either GDT
 *        or LDT. (0: GDT, 1: LDT)
 *   3-15 bits: 12 bit length index 
 *        since length index starts at bit 3, its value is multiple of 8
 *        and our descriptor entries are also 8 bytes.
 *        index + gdt base address = descriptor entry
 *        our code segment descriptor is the 2nd entry
 *        we cannot change cs register directly except to use 'call' or
 *        'jmp'.
 *        example: jmp (0x0001 << 3) : offset
 * 
 * ---------------------------------------------------------------------------
 * GDT Entry
 *
 * a segment descriptor is an 8-byte structure that defines the following
 * properties of a protected-mode segment:
 *   1. base address (32 bits), which defines where the segment begins in
 *      physical memory
 *   2. segment limit (20 bits), which defines the size of the segment
 *   3. various flags, which affect how the CPU interprets the segment,
 *      such as the privilige level of code that runs within it or 
 *      whether it is read or write-only.
 *      
 *
 * structure of an GDT entry
 *  15                                                                0
 * ┌───────────────────────────────────────────────────────────────────┐
 * │                      segment limit 00-15                          │
 * └───────────────────────────────────────────────────────────────────┘ 
 *  15                                                                0
 * ┌───────────────────────────────────────────────────────────────────┐
 * │                      segment base 00-15                           │
 * └───────────────────────────────────────────────────────────────────┘
 *  7                               0
 * ┌─────────────────────────────────┐
 * │       segment base 16-23        │
 * └─────────────────────────────────┘
 *  7   6    5   4   3   2    1     0
 * ┌───┬───────┬───┬───┬───┬─────┬───┐
 * │ P │  DPL  │ S │ X │ C │ R/W │ A │
 * └───┴───────┴───┴───┴───┴─────┴───┘ 
 *  7    6   5   4  3               0
 * ┌───┬───┬───┬───┬─────────────────┐
 * │ G │ D │ L │ V │ seg limit 16-19 │
 * └───┴───┴───┴───┴─────────────────┘
 *  7                               0
 * ┌─────────────────────────────────┐
 * │       segment base 24-31        │
 * └─────────────────────────────────┘
 *
 * only few of those bits mentioned here
 *
 * DPL: Descriptor Privilege Level (to which ring it is attached.)
 *
 * G: Granularity, 0 = byte, 1 = 4KB.
 *    if G = 1, limit = 0xfffff which means segment cover 4 GB space
 *
 * X: Executable bit, 1: code, 0
 *    X = 1 code segment
 *    X = 0 data segment
 *
 * C: Conforming Code Segment
 *    C = 1, conforming code segment.
 *    code in this segment can be executed from an equal or lower
 *    privilege level.
 *        for example: code in ring 3 can far-jump to conforming code in
 *                     a ring 2 segment.  
 *    DPL-bits represent the highest privilege level that is allowed to
 *    execute the segment. 
 *       for example: code in ring 0 cannot far-jump to a conforming code
 *                    segment with DPL = 0, while code in ring 2 and 3
 *                    can.
 *
 *   note: that the privilege level remains the same, i.e. a far-jump
 *         from ring 3 to DPL = 1 segment, CPL remains in ring 3 after
 *         the jump.
 *
 *   C = 0, nonconforming code segment. code in this segment can only
 *          be executed from the code which CPL = DPL
 *
 * S: Descriptor type 
 *    S = 0, system type 
 *       e.g.
 *       TSS Descriptor, LDT Descriptor, Gate Descriptors (Call, Trap,
 *       Interrupt, Task)
 *    S = 1, code/data type
 *    Notice:
 *       when S = 1 (code/data), the [D,C,RW,A] 4 bits can be interpreted
 *                               separately
 *       when S = 0 (system), processor treat them as 'System-Segment-
 *                            Descriptor-Types'
 *           for example: 0x2 means this descriptor is a LDT descriptor
 *           more information: 
 *               Intel Developer's Manual Vol.3 Table 3-2.
 *               AMD Programmer's Manual Vol.2 -> Table 4-5.
 *
 * for more information about GDT: 
 * #URL2 https://wiki.osdev.org/Global_Descriptor_Table
 * 
 * 
 * the CPU requires that first entry in the GDT purposely be an invalid
 * 'null' descriptor. the 'null' descriptor is a simple mechanism to
 * catch mistabkes where we forget to set a particular segment register
 * before accessing an address, which is easily done if we had some
 * segment registers set to '0' and forgot to update them to the 
 * appropriate segment descriptors after switching to protected mode.
 * if an addressing attempt is made with the 'null' descriptor, then the
 * CPU raise an exception, which essentially is an interrupt.
 * 
 * 
 * 
 * a segment selector to this 'null descriptor' does not generate an 
 * exception when loaded into a data-segment register (DS, ES, FS, or
 * GS), but it always generates a general-protection exception (#GP) when
 * an attempt is made to access memory using the descriptor.
 *
 * ---------------------------------------------------------------------------
 * ---------------------------------------------------------------------------
 * CPU has a register 'gdtr'(GDT Register) to save the information of
 * a GDT.
 * 
 * GDTR is a 48-bit register
 *  47                          16 15                 0
 * ┌──────────────────────────────┬────────────────────┐
 * │  32-bit Linear Base Address  │ 16-bit Table Limit │
 * └──────────────────────────────┴────────────────────┘
 *
 * use 'lgdt' to load it.
 *   lgdt    [global_descriptor_table_info]
 * end of using lgdt
 * ------------------------------------------------------------------------ */

#pragma pack(push, 1)

struct gdt_desc_t
{
    uint16_t _size;
    uint32_t _address;
};

using idt_desc_t = gdt_desc_t;

// generic segment descriptor
class code_desc_t
{
public:
    uint16_t _limit_l;     // limit 0-15
    uint16_t _base_l;      // base 0-15
    uint8_t  _base_m;      // base 16-23
    uint8_t  _type    : 4, // segment type
             _s       : 1, // 0:system(tss/idt) or 1:code/data segment
             _dpl     : 2, // desc privilege level
             _p       : 1; // present
    uint8_t  _limit_h : 4, // limit 16-19
             _avl     : 1, // available to user(sys. programmer)
             _l       : 1, // 64-bit flag
             _d       : 1, // default (AMD: reserved)
             _g       : 1; // granularity
    uint8_t  _base_h;      // base 23-31

    enum
    {
        DEF_CODE_TYPE = 0b1010,
        DEF_DATA_TYPE = 0b0010
    };

    void reset()
    {
        _limit_l = 0;
        _base_l  = 0;
        _base_m  = 0;
        _type    = 0;
        _s       = 1; // code/data segment
        _dpl     = 0;
        _p       = 0;
        _limit_h = 0;
        _avl     = 0;
        _l       = 0;
        _d       = 0;
        _g       = 1; // 4K is default
        _base_h  = 0;
    }

    void initialize(uint8_t  type,
                    uint32_t base,
                    uint32_t limit,
                    uint8_t  dpl)
    {
        _s    = 1; // which means it's code/data segment
        _type = type;
        _dpl  = dpl;
        set_limit(limit);
        set_base(base);
    }

    void set_base(uint32_t base) {
        _base_l = base & 0xffff;
        _base_m = (base >> 16) & 0xff;
        _base_h = (base >> 24) & 0xff;
    }

    uint32_t get_base() const {
        return _base_l | _base_m << 16 | _base_h << 24;
    }

    void set_limit(uint32_t limit) {
        _limit_l = limit & 0xffff;
        _limit_h = (limit >> 16) & 0xf;
    }

    uint32_t get_limit() const {
        return _limit_l | (_limit_h << 16);
    }
};

// data segment descriptor
using data_desc_t = code_desc_t;

#pragma pack(pop)
