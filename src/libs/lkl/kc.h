#include <lkl.h>

ns_lite_kernel_lib_begin

class k_creator
{
private:
    bool _initialized : 1 = false;
private:

    // Low Memory Layout
    enum
    {
        LML_IVT_BASE      = 0x00000000, 
        LML_IVT_SIZE      = 0x00000400, 

        LML_BDA_BASE      = LML_IVT_BASE      + LML_IVT_SIZE, 
        LML_BDA_SIZE      = 0x00000100, 
        
        LML_RSRV1_BASE    = LML_BDA_BASE      + LML_BDA_SIZE, 
        LML_RSRV1_SIZE    = 0x00001B00, 
        
        LML_GDT_BASE      = LML_RSRV1_BASE    + LML_RSRV1_SIZE, 
        LML_GDT_SIZE      = 0x00000100, 
        
        LML_IDT_BASE      = LML_GDT_BASE      + LML_GDT_SIZE,
        LML_IDT_SIZE      = 0x00000200, 
        
        LML_TSS_BASE      = LML_IDT_BASE      + LML_IDT_SIZE,
        LML_TSS_SIZE      = 0x00000200,
        
        LML_ARDS_BASE     = LML_TSS_BASE      + LML_TSS_SIZE, 
        LML_ARDS_SIZE     = 0x00000200, 
        
        LML_RSRV2_BASE    = LML_ARDS_BASE     + LML_ARDS_SIZE, 
        LML_RSRV2_SIZE    = 0x00000900, 
        
        LML_POOL_BUF_BASE = LML_RSRV2_BASE    + LML_RSRV2_SIZE, 
        LML_POOL_BUF_SIZE = 0x00032000, 
        
        LML_RSRV3_BASE    = LML_POOL_BUF_BASE + LML_POOL_BUF_SIZE, 
        LML_RSRV3_SIZE    = 0x0004B000, 

        // not conntected with above
        LML_KSTACK_BASE   = 0x0009F000, 
        LML_KSTACK_SIZE   = 0x00001000, 
    };

private:
    k_creator() = default;
public:
    k_creator(k_creator&&)           = delete;
    k_creator(const k_creator&)      = delete;
    void operator=(k_creator&&)      = delete;
    void operator=(const k_creator&) = delete;
public:
    bool
    initialize();

public:
    inline bool
    initialized() const {
        return _initialized;
    }

public:
    static k_creator&
    instance() {
        static k_creator s_creator;
        return s_creator;
    }
private:
    void
    invoke_global_ctors();

    void
    turn_on_paging();

    void
    init_tss();

private:
    void
    __inner_show_welcome();
};



ns_lite_kernel_lib_end