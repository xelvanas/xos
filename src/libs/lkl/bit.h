#pragma once
#include <lkl.h>

ns_lite_kernel_lib_begin

template<typename T, typename U>
inline void 
bit_set(T& r, U s, bool b) {
    r = b ? r | (T)s : r & (T)~s;
}

// this won't change value of first parameter
template<typename T, typename U>
inline T 
bit_ret(T r, U s, bool b) {
    return b ? r | (T)s : r & (T)~s;
}

template<typename T, typename U>
inline bool
bit_test(const T r, U s) {
    return (r & (T)s) != 0;
}

template<typename T, typename U>
inline void
bit_toggle(T& r, U s) {
    bit_set(r, s, !bit_test(r, s));
}

ns_lite_kernel_lib_end
