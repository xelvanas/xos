#pragma once
#include <lkl.h>

ns_lite_kernel_lib_begin

template<typename T, typename U>
inline void 
bit_set(T& r, U s, bool b) {
    r = b ? r | (T)s : r & (T)~s;
}

template<typename T, typename U>
inline bool 
bit_test(const T r, U s) {
    return (r & (T)s) != 0;
}

ns_lite_kernel_lib_end
