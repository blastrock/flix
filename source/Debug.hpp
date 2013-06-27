#ifndef DEBUG_HPP
#define DEBUG_HPP

#include "Screen.hpp"
#include "Dmesg.hpp"

#define fDeg() \
  Dmesg()
  //Dmesg() << __FILE__ << ':' << __LINE__ << ": "

#define fInfo() \
  Dmesg()

inline void PANIC(const char* str)
{
  Screen::putString("kernel panic:\n");
  Screen::putString(str);
  asm volatile ("cli");
  while (true)
    asm volatile ("hlt");
}

#define debug(str, i) \
  fDeg() << str << ": " << std::hex << i;

void printStackTrace(uint64_t stackPointer);

#endif /* DEBUG_HPP */
