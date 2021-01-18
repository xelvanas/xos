#pragma once
#include <lkl.h>
#include <pool.h>
#include <lock.h>
#include <debug.h>

ns_lite_kernel_lib_begin

class space_mgr
{
private:
    pool_t _kpool;
    pool_t _upool;
    lock_t _lock;
public:

    // space type
    enum class spt_t
    {
        SPT_KERNEL,
        SPT_USER
    };

    inline void
    reset_pool_buf(spt_t st,
                   void*        buf,
                   uint32_t     buf_size,
                   uint32_t     base,
                   uint32_t     space_size) 
    {
        lock_guard guard(_lock);

        if(st == spt_t::SPT_KERNEL) {
            _kpool.reset(
                buf,
                buf_size,
                base,
                space_size
            );
        } else {
            _upool.reset(
                buf,
                buf_size,
                base,
                space_size
            );
        }
    }

    // inline void*
    // get_pool_buf


private:

};

// class va_mgr
// {
//     pool_t _kv_pool;
//     pool_t _uv_pool;
//     lock_t _lock;
// public:
//     // inline bool
//     // init(void*   us,  // starting virtual address of user space
//     //     uint32_t un,  // page count of user space
//     //     void*    ks,  // starting virtual address of kernel space
//     //     uint32_t kn)  // page count of kernel space
//     // {
//     //     uint32_t ua = (uint32_t)us;
//     //     uint32_t ka = (uint32_t)ks;

//     //     // user space and kernerl space cannot be overlapped
//     //     // if(!__inner_overlapped_test(ua, un*PAGE_SIZE, ka, kn*PAGE_SIZE))
//     //     //     return false;

//     // }


// private:

//     // inline bool
//     // __inner_overlapped_test(
//     //     uint32_t ua,
//     //     uint32_t ul,
//     //     uint32_t ka,
//     //     uint32_t kl)
//     // {
//     //     bool ret = false;
//     //     ret |= __inner_in_range_test(ua, ul, ka);
//     //     ret |= __inner_in_range_test(ua, ul, ka + kl);
//     //     ret |= __inner_in_range_test(ka, kl, ua);
//     //     ret |= __inner_in_range_test(ka, kl, ua + ul);
//     //     return ret;
//     // }

//     // inline bool
//     // __inner_in_range_test(
//     //     uint32_t start,
//     //     uint32_t len,
//     //     uint32_t addr)
//     // {
//     //     uint32_t end = start + len;
//     //     return addr >= start && addr < end;
//     // }
// };




ns_lite_kernel_lib_end