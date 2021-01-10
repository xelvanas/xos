#include <string.h>
#include <errno.h>
#include <debug.h>

int memcpy_s(
    void*       __dst,
    size_t      __dstsz,
    const void* __src,
    size_t      __srcsz)
{

    if(__srcsz == 0) {
        // this is not an error
        return 0;
    }

    if(__dst == nullptr || __src   == nullptr) {
        return EINVAL;
    }

    if(__dstsz < __srcsz) {
        return ERANGE;
    }

    char* dst = (char*)__dst;
    const char* src = (const char*)__src;
    for(size_t i = 0; i < __srcsz; ++i) {
        dst[i] = src[i];
    };
    return 0;
}

void* memset(
    void*  __dst,
    int    __val,
    size_t __sz) 
{
    if(__dst != nullptr && __sz > 0) {
        auto dst = (uint8_t*)__dst;
        for(size_t i = 0; i < __sz; ++i) {
            dst[i] = (uint8_t)__val;
        }
    }
    return __dst;
}


int strcpy_s(
        char*       __dst,
        size_t      __dstsz,
        const char* __src
        )
{
    if(__dst == nullptr) {
        return EINVAL;
    }

    if(__src == nullptr) {
        __dst[0] = 0;
        return EINVAL;
    }

    if(__dstsz != 0)
    {
        for(size_t i = 0; i < __dstsz; ++i) {
            __dst[i] = __src[i];
            if(__src[i] == 0) {
                return 0;
            }
        }
    }

    __dst[0];
    return ERANGE;
}

size_t strlen(const char* __str) {
    ASSERT(__str != nullptr);
    const char* ptr = __str;
    while(*ptr++);
    return ptr - __str - 1;
}

const char* strchr(
    const char* __str,
    int c)
{
    ASSERT(__str != nullptr);
    while(*__str != 0) {
        if(*__str == (char)c) {
            return __str;
        }
    }
    return nullptr;
}

int strcat_s(
        char*       __dst,
        size_t      __dstsz,
        const char* __src)
{
    if(__dst == nullptr ||
       __src == nullptr)
        return EINVAL;

    size_t len = strlen(__dst);
    if(len < __dstsz) {
        __dst   += len;
        __dstsz -= len;
        for(size_t i = 0; i < __dstsz; ++i) {
            __dst[i] = __src[i];
            if(__dst[i] == 0) {
                return 0;
            }
        }
    }
    __dst[0] = 0;
    return ERANGE;
}

