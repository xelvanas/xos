#pragma once
#include <lkl.h>
#include <debug.h>

/*
 * Detecting Memory
 * By far the best way to detect the memory of a PC is by using the 
 * 'INT 0x15', 'EAX=0xE820' command. this function is available on all
 * PCs built since 2002, and on most existing PCs before then. It is the
 * only BIOS function that can detect memory areas above 4G. it is meant
 * to be the ultimate memory detection BIOS function.
 *   
 * in reality, this function returns an unsorted list that may contain 
 * unused entries and (in rare/dodgy cases) may return overlapping areas.
 * each list entry is stored in memory at ES:DI, and DI is not 
 * incremented for you. the format of an entry is 2 unit64_t and a 
 * unit32_t in  the 20 byte version, plus additional unit32_t in the 24
 * 
 * byte ACPI 3.0 version (but nobody has ever seen a 24 byte one). it is
 * probably best to always store the list entries as 24 byte quantities -
 * to preserve uint64_t alignments, if nothing else. (make sure to set 
 * that last unit64_t to 1 before each call, to make your map compatible
 * with ACPI).
 *  
 *  ARDS (Address Range Descriptor Structure)
 *  offset     property
 * ┌─────┬───────────────────────────┐
 * │  0  │     Base Address Low      │
 * ├─────┼───────────────────────────┤
 * │  4  │     Base Address High     │
 * ├─────┼───────────────────────────┤
 * │  8  │     Length Low            │
 * ├─────┼───────────────────────────┤
 * │  12 │     Length High           │
 * ├─────┼───────────────────────────┤
 * │  16 │     Type                  │
 * └─────┴───────────────────────────┘
 * Base Address: where the region begins
 * Length: size of the "region" (if this value is 0, ignore the entry)
 * Type:
 *     type 1: Usable (normal) RAM
 *     type 2: Reserved - unusable
 *     type 3: ACPI reclaimable memory
 *     type 4: ACPI NVS memory
 *     type 5: Area containing bad memory
 *
 * Basic Usage
 * for the first call to the function:
 *   1. point ES:DI at the destination buffer for the list. 
 *   2. clear 'EBX'.
 *   3. set 'EDX' to the magic number 0x534D4150.
 *   4. set 'EAX' to 0xE820 (note that the upper 16-bits of 'EAX' should
 *      be set to 0).
 *   5. set 'ECX' to 24.
 *   6. do an 'INT 0x15'
 * 
 * if the first call to the function is successful:
 *   1. 'EAX' will be set to 0x534D4150, 
 *   2. the 'Carry Flag' will be clear. 
 *   3. 'EBX' will be set to some non-zero value, which must be preserved
 *      for the next call to the function.
 *   4. 'CL' will contain the number of bytes actually stored at 'ES:DI'
 *      (probably 20).
 *   5. reached the end if 'EBX=0" and 'Carry Flag' is zero.
 * 
 * for the subsequent calls to the function: 
 *   1. increment 'DI' by your list entry size.
 *   2. reset 'EAX' to 0xE820
 *   3. and ECX to 24. 
 * when you reach the end of the list, 'EBX' may reset to 0. if you call
 * the function again with 'EBX=0', the list will start over. if 'EBX'
 * does not reset to 0, the function will return with 'Carry' set when
 * you try to access the the entry after last valid entry.
 * 
 * 
 * 
 *
 * Note:
 *   1. after getting the list, it may be desirable to: sort the list,
 *      combine adjacent ranges of the same type, change any overlapping
 *      areas to the most restrictive type, and change any unrecognised
 *      'type' to 'type 2'.
 *      
 *   2. type 3 'ACPI reclaimable' memory regions may be used like
 *      (and combined with) normal 'available RAM' areas as long as
 *      you're finished using the ACPI tables that are stored here.
 *      
 *   3. types 2, 4, 5(reserved, ACPI non-volatile, bad) mark areas that
 *      should be avoided when you are allocating physical memory.
 *      
 *   4. treat unlisted regions as 'type 2' - reserved.
 *   5. your code must be able to handle areas that don't start or end on
 *      any sort of 'page boundary'
 *
 * typical output by calling 'INT 0x15, EAX=0xE820' in bochs (megs: 32):
 *  Base Address       | Length             | Type
 *  0x0000000000000000 | 0x000000000009FC00 | Free Memory (1)
 *  0x000000000009FC00 | 0x0000000000000400 | Reserved Memory (2)
 *  0x00000000000E8000 | 0x0000000000018000 | Reserved Memory (2)
 *  0x0000000000100000 | 0x0000000001F00000 | Free Memory (1)
 *  0x00000000FFFC0000 | 0x0000000000040000 | Reserved Memory (2)
 */

ns_lite_kernel_lib_begin

/*
 * 'Memory Detecting' has already been done at 'boot stage'.
 * normally, we don't change the value of ards_t.
 */
class ards_t
{
private:
    uint64_t    _address;
    uint64_t    _length;
    uint32_t    _type;
public:
    enum
    {
        T_USABLE    = 0x01, // Usable (normal) memory
        T_RESERVED  = 0x02, // Reserved - unusable
        T_ACPI_RECL = 0x03, // ACPI reclaimable memory
        T_ACPI_NVS  = 0x04, // ACPI NVS memory
        T_ACBM      = 0x05, // Area containing bad memory
    };

    enum
    {
        ARDS_NUMB_ADDR  = 0x0800,
        ARDS_DATA_ADDR  = 0x0804,
    };

    void set(uint64_t addr, uint64_t len, uint32_t type) {
        _address = addr;
        _length  = len;
        _type    = type;
    }

    void set_type(uint32_t type) {
        _type = type;
    }

    void set_address(uint64_t addr) {
        _address = addr;
    }

    void set_length(uint64_t len) {
        _length = len;
    }

    bool is_usable() const {
        return _type == T_USABLE;
    }

    uint64_t address() const {
        return _address;
    }

    uint64_t length() const {
        return _length;
    }
    
    uint32_t type() const {
        return _type;
    }

    void info() {
        // in 32-bit mode
        dbg_msg("ARDS:\n    address:");
        dbg_hex((uint32_t)_address);
        dbg_msg("\n    length:");
        dbg_hex((uint32_t)_length);
        dbg_msg("\n    type:");
        dbg_hex(_type);
        dbg_ln();
    }
};

ns_lite_kernel_lib_end
