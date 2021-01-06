#pragma once

void panic_spin(
    const char* filename,
    int line,
    const char* func,
    const char* condition);

#define PANIC(...) panic_spin (__FILE__, __LINE__, __func__, __VA_ARGS__)

#ifdef NDEBUG
   #define ASSERT(CONDITION) ((void)0)
#else
   #define ASSERT(CONDITION)        \
      if (CONDITION) {} else        \
      {                             \
        PANIC(#CONDITION);          \
      }
#endif

