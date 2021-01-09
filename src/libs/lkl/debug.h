#pragma once
extern "C"
{
   void panic_spin(
      const char* filename,
      int line,
      const char* func,
      const char* condition);

   void dbg_msg( const char* msg, uint8_t col = 0x07); //0x07:light gray
   void dbg_hex(uint32_t val, uint8_t col = 0x07);
   void dbg_ln();
}

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
