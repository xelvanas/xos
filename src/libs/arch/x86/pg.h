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

inline extern const int PAGE_SIZE   = 0x1000;
inline extern const int PD_ENT_NUM  = 0x0400; // page-directory entry num
inline extern const int PT_ENT_NUM  = 0x0400; // page-table entry num

struct raw_pde_t
{
    bool     p       :  1; // present
    bool     rw      :  1; // read/write
    bool     us      :  1; // user/supervisor
    bool     pwt     :  1; // page-level write through
    bool     pcd     :  1; // page-level cache disable
    bool     a       :  1; // accessed
    bool     ign1    :  1; // ignored
    bool     ps      :  1; // page size, used when cr4.pse = 1
    uint8_t  ign2    :  4; // ignored
    uint32_t address : 20; // physical address of 4k aligned page table
};

struct raw_pte_t
{
    bool     p       :  1; // present
    bool     rw      :  1; // read/write
    bool     us      :  1; // user/supervisor
    bool     pwt     :  1; // page-level write through
    bool     pcd     :  1; // page-level cache disable
    bool     a       :  1; // accessed
    bool     d       :  1; // dirty
    bool     pat     :  1; // page attribute table
    bool     g       :  1; // global
    uint8_t  ign2    :  3; // ignored
    uint32_t address : 20; // physical address of 4k page 
};

// page generic entry
// can present both pde and pte
class pge_t
{
protected:
    union
    {
        raw_pde_t _pde;
        raw_pte_t _pte;
        uint32_t  _value;
    };
public:
    enum
    {
        MASK_LO_12BITS = 0x0000'0FFF,
        MASK_HI_20BITS = 0xFFFF'F000
    };

public:
    pge_t() = default;
    pge_t(uint32_t val)
        : _value(val) {

    }

    inline void
    zeroize() {
        _value = 0;
    }

    // must be true to map a 'page' or 'page table' 
    bool
    present() const {
        return _pde.p;
    }
    
    // set whether 'page'/'page table' referenced by this entry valid.
    void
    present(bool p) {
        _pde.p = p;
    }

    // false: read only
    // true:  writes allowed
    bool
    writable() const {
        return _pde.rw;
    }

    // true:  writable
    // false: read only
    void
    writable(bool v) {
        _pde.rw = v;
    }

    // false: supervisor-mode
    // true:  user-mode
    bool
    usr() const {
        return _pde.us;
    }

    // false: supervisor-mode
    // true:  user-mode
    void
    usr(bool v) {
        _pde.us = v;
    }

    // page write through state
    bool
    pwt() const {
        return _pde.pwt;
    }

    // set page write through state
    void
    pwt(bool p) {
        _pde.pwt = p;
    }

    // page level cache disable 
    bool
    pcd() const {
        return _pde.pcd;
    }

    // set page level cache disable
    void
    pcd(bool p) {
        _pde.pcd = p;
    }

    // test if entry accessed by software
    bool
    accessed() const {
        return _pde.a;
    }

    // set page accessed state
    // void
    // accessed(bool a) {
    //     _pde.a = a;
    // }

    // set physical address
    void
    address(uint32_t addr) {
        _pde.address = addr >> 12;
    }

    // get physical address
    uint32_t
    address() const {
        return _pde.address << 12;
    }

    operator uint32_t() const {
        return _value;
    }

    const pge_t&
    operator=(uint32_t val) {
        _value = val;
        return *this;
    }

    static inline uint32_t
    get_pde_index(uint32_t addr) {
        return ((addr & 0xffc00000) >> 22);
    }

    static inline uint32_t
    get_pte_index(uint32_t addr) {
        return ((addr & 0x003ff000) >> 12);
    }
};

class pde_t : public pge_t
{
public:
    pde_t() = default;
    pde_t(uint32_t val)
        : pge_t(val) {

    }
    // page size
    // if cr4.PSE = 1, must be 0 (otherwise, this entry maps 4-MByte page);
    // otherwise: ignored
    bool ps() const {
        return _pde.ps;
    }

    // set page size
    // PDE only
    void ps(bool p) {
        _pde.ps = p;
    }
};

class pte_t : public pge_t
{
public:
    pte_t() = default;
    pte_t(uint32_t val)
        : pge_t(val) {

    }
    // Dirty
    // indicates whether software was written to this 4-Kbyte page
    // referenced by this entry
    // can be used as a guard to detect overflow
    bool dirty() const {
        return _pte.d;
    }

    // not allowed to write this value yet
    // void dirty(bool d) {
    //     lkl::bit_set(_value, PG_DIRTY, d);
    // }

    // Page Attribute Table
    // if the PAT is supported, indirectly determines the memory type used
    // to access the 4-KByte page referenced by this entry
    bool pat() const {
        return _pte.pat;
    }

    // Page Attribute Table
    // if the PAT is supported, indirectly determines the memory type used
    // to access the 4-KByte page referenced by this entry
    void pat(bool p) {
        _pte.pat = p;
    }

    // Global:
    // if cr4.PGE = 1, determines whether the translation is global;
    // otherwise: ignored.
    bool global() const {
        return _pte.g;
    }

    // Global:
    // if cr4.PGE = 1, determines whether the translation is global;
    // otherwise: ignored.
    void global(bool g) {
        _pte.g = g;
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
              uint32_t step = PAGE_SIZE)
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
// using pde_t      = pge_t;
// using pte_t      = pge_t;
using page_dir_t = pea_t;
using page_tbl_t = pea_t;

#pragma pack(pop)