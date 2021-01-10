#pragma once
#include <stddef.h>
#include <stdint.h>

extern "C"
{
    int memcpy_s(
        void*       __dst,
        size_t      __dstsz,
        const void* __src,
        size_t      __srcsz
    );

    void* memset(
        void*  __dst,
        int    __val,
        size_t __sz
    );

    int strcpy_s(
        char*       __dst,
        size_t      __dstsz,
        const char* __src
    );

    size_t strlen(
        const char* __str
    );

    const char* strchr(
        const char* __str,
        int         __c
    );

    int strcat_s(
        char*       __dst,
        size_t      __dstsz,
        const char* __src
    );
}
