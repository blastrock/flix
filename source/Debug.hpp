#ifndef DEBUG_HPP
#define DEBUG_HPP

#include "Screen.hpp"
#include "Dmesg.hpp"


inline void PANIC(const char* str)
{
  Screen::putString("kernel panic:\n");
  Screen::putString(str);
  asm volatile ("cli");
  while (true)
    asm volatile ("hlt");
}

void printStackTrace(uint64_t stackPointer);

#endif /* DEBUG_HPP */
