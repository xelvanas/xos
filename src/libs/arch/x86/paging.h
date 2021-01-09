#pragma once
#include <stdint.h>
#include <debug.h>
#include <bit.h>

/* ---------------------------------------------------------------------------
 * 32-Bit Paging
 * a logical processor uses 32-bit paging if CR0.PG = 1 and CR4.PAE = 0.
 * 32-bit paging translates 32-bit linear addresses to 40-bit physical 
 * addresses. although 40 bits corresponds to 1 TByte, linear addresses
 * are limited to 32 bits; at most 4 GByte of linear address space may be
 * accessed at any given time.
 * 
 * 32-bit paging uses a hierarchy of paging structures to produce a 
 * translation for a linear address. CR3 is used to locate the first
 * paging-structure, the 'page directory'.
 *
 * CR3 Register
 *  31                                12 11         5   4   3  2  1  0
 * ┌────────────────────────────────────┬─────────────┬───┬───┬───────┐
 * │     Address of Page Directory      │     IGN     │PCD│PWT│  IGN  │
 * └────────────────────────────────────┴─────────────┴───┴───┴───────┘
 * PWT:  Page-level write-through; indirectly determines the memory type
 *       used to access the page directory during linear-address
 *       translation.
 * PCD:  Page-level cache disable; indirectly determines the memory type
 *       used to access the page directory during linear-address
 *       translation.
 *       
 * 12-31 bits: Physical address of the 4-KByte aligned page directory
 *       used for linear address-translation.
 *       
 *
 * Page Directory Entry (PDE)
 *  31                              12   11  7  6  5  4   3   2   1  0
 * ┌──────────────────────────────────┬─────┬─┬───┬─┬───┬───┬───┬───┬─┐
 * │     Address of Page Table        │ IGN │0│IGN│A│PCD│PWT│U/S│R/W│P│
 * └──────────────────────────────────┴─────┴─┴───┴─┴───┴───┴───┴───┴─┘
 * P:    Present; must be 1 to reference a page table
 * R/W:  Read/Write; if 0, writes may not be allowed to the 4-MByte
 *       region controlled by this entry.
 * U/S:  User/Supervisor; controls access to the page based on privilege
 *       level. if 1, then page may be accessed by all. if 0, only the
 *       supervisor can access it.
 * PWT:  page-level write-through; indirectly determines the memory type
 *       used to access the page table referenced by this entry.
 * PCD:  page-level cache disable; indirectly determines the memory type
 *       used to access the page table referenced by this entry.
 * A:    Accessed; indicates whether this entry has been used for linear
 *       address translation.
 * Bit7: if CR4.PSE=1, must be 0 (otherwise this entry has been maps a 
 *       4-MByte page) otherwise, ignored.
 *
 * Page Table Entry (PTE)
 *  31                               12   11   7  6 5  4   3   2   1  0
 * ┌───────────────────────────────────┬─────┬───┬─┬─┬───┬───┬───┬───┬─┐
 * │     Address of Page               │ IGN │PAT│D│A│PCD│PWT│U/S│R/W│P│
 * └───────────────────────────────────┴─────┴───┴─┴─┴───┴───┴───┴───┴─┘
 * P:    Present; must be 1 to map a 4-KByte page
 * R/W:  Read/Write; if 0, writes may not be allowed to the 4-KByte
 *       referenced by this entry.
 * U/S:  User/Supervisor; if 0, user-mode accesses are not allowed to
 *       the 4-KByte page referenced by this entry.
 * PWT:  Page-level write-through; indirectly determines the memory type
 *       used to access the 4-KByte page referenced by this entry.
 * PCD:  Page-level cache disable; indirectly determines the memory type
 *       used to access the 4-KByte page referenced by this entry.
 * A:    Accessed; indicates whether software has written to the 4-KByte
 *       page referenced by this entry.
 * D:    Dirty; indicates whether software has written to the 4-KByte
 *       page referenced by this entry.
 * PAT:  if the PAT is supported, indirectly determines the memory type
 *       used to access the 4-KByte page referenced by this entry.
 * G:    Global, if CR4.PGE = 1, determines whether the translation is
 *       global, ignored otherwise.
 * ------------------------------------------------------------------------ */


#pragma pack(push, 1)

// page generic entry
// can present both pde and pte
class pge_t
{
private:
    uint32_t _value;
public:
    enum
    {
        PG_PRESENT      = 0x00000001, // present
        PG_RW           = 0x00000002, // read/write
        PG_USER         = 0x00000004, // user mode if set
        PG_SUPERVISOR   = 0x00000004, // supervisor if not set
        PG_PWT          = 0x00000008, // page write through
        PG_PCD          = 0x00000010, // page-level cache disable
        PG_ACCESSED     = 0x00000020, // leave it to CPU
        PG_DIRTY        = 0x00000040, // dirty [PTE only]
        PG_SIZE         = 0x00000080, // page size [PDE only]
        PG_PAT          = 0x00000080, // page attribute table [PTE only]
        PG_GLOBAL       = 0x00000100, // global if CR4.PGE = 1[PTE only]
        PG_12BIT_FLAG   = 0x00000FFF,
        PG_ADDRESS      = 0xFFFFF000
    };

    // get present
    bool present() const {
        return lkl::bit_test(_value, PG_PRESENT);
    }
    
    // set present
    void present(bool p) {
        lkl::bit_set(_value, PG_PRESENT, p);
    }

    // get readonly
    bool ro() const {
        return lkl::bit_test(_value, PG_RW);
    }

    // set readonly
    void ro(bool r) {
        lkl::bit_set(_value, !r, PG_RW);
    }

    // get user mode
    bool usr() const {
        return lkl::bit_test(_value, PG_USER);
    }

    // set user mode
    void usr(bool u) {
        lkl::bit_set(_value, PG_USER, u);
    }

    // get supervisor mode
    // user-mode accesses are not allowed if it is 'true'
    bool sup() const {
        return lkl::bit_test(_value, PG_SUPERVISOR) == false;
    }

    // set supervisor mode
    // sup(true) -> flag = 0, user-mode accesses are not allowed.
    void sup(bool s) {
        lkl::bit_set(_value, PG_SUPERVISOR, !s);
    }

    // get page write through
    bool pwt() const {
        return lkl::bit_test(_value, PG_PWT);
    }

    // set page write through
    void pwt(bool p) {
        lkl::bit_set(_value, PG_PWT, p);
    }

    // get page level cache disable
    bool pcd() const {
        return lkl::bit_test(_value, PG_PCD);
    }

    // set page level cache disable
    void pcd(bool p) {
        lkl::bit_set(_value, PG_PCD, p);
    }

    // get page accessed
    bool accessed() const {
        return lkl::bit_test(_value, PG_ACCESSED);
    }

    // set page accessed
    void accessed(bool a) {
        lkl::bit_set(_value, PG_ACCESSED, a);
    }

    // get dirty
    // PTE only
    bool dirty() const {
        return lkl::bit_test(_value, PG_DIRTY);
    }

    // set dirty
    // PTE only
    void dirty(bool d) {
        lkl::bit_set(_value, PG_DIRTY, d);
    }

    // get page size
    // PDE only
    bool ps() const {
        return lkl::bit_test(_value, PG_SIZE);
    }

    // set page size
    // PDE only
    void ps(bool p) {
        lkl::bit_set(_value, PG_SIZE, p);
    }

    // get page attribute table
    // PTE only
    bool pat() const {
        return lkl::bit_test(_value, PG_PAT);
    }

    // set page attribute table
    // PTE only
    void pat(bool p) {
        lkl::bit_set(_value, PG_PAT, p);
    }

    // get page attribute table
    // PTE only
    bool global() const {
        return lkl::bit_test(_value, PG_GLOBAL);
    }

    // set page attribute table
    // PTE only
    void global(bool g) {
        lkl::bit_set(_value, PG_GLOBAL, g);
    }

    // set physical address
    void address(uint32_t addr) {
        _value = (_value&PG_12BIT_FLAG) | (addr&PG_ADDRESS);
    }

    // get physical address
    uint32_t address() const {
        return _value & PG_ADDRESS;
    }

    operator uint32_t() const { return *(uint32_t*)this; }

    const pge_t& operator=(uint32_t val) {
        *(uint32_t*)this = val;
        return *this;
    }
};

// page entry array
class pea_t
{
    uint32_t _len;
    pge_t*   _array;
public:

    // default constructor
    pea_t() : _len(0), _array(nullptr) {

    }
    
    pea_t(pge_t* addr, uint32_t len) :
    _array(addr),
    _len(len) {

    }

    void reset(pge_t* addr, uint32_t len) {
        _array = addr;
        _len   = len;
    }

    pge_t& operator[](uint32_t idx) {
        ASSERT(idx < _len);
        return _array[idx];        
    }

    const pge_t& operator[](uint32_t idx) const {
        ASSERT(idx < _len);
        return _array[idx];        
    }

    // default step is 0x1000 (4KB)
    void fill(uint32_t start, 
              uint32_t end,
              uint32_t value,
              uint32_t step = 0x1000)
    {
        if(start >= _len && end > _len)
            return;

        for (uint32_t i = start; i < end; i++) {
            _array[i] = value;
            value    += step;
        }
    }

    void copy(pge_t* src, uint32_t start, uint32_t len) {
        if(src == nullptr || start >= _len)
            return;

        if(start + len > _len) {
            len = _len - start;
        }

        uint32_t end = start + len > _len ?
                       _len :
                       start + len;

        for(uint32_t i = 0; i < len;++i) {
            _array[start+i]  = src[i];
        }
    }
};

// aliases
using pde_t = pge_t;
using pte_t = pge_t;
using page_dir_t = pea_t;
using page_tbl_t = pea_t;

#pragma pack(pop)