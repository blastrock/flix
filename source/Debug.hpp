#ifndef DEBUG_HPP
#define DEBUG_HPP

#include "Screen.hpp"
#include "Dmesg.hpp"

#define fDeg() \
  Dmesg() << __FILE__ << ':' << __LINE__ << ": "

#define fInfo() \
  Dmesg()

inline void PANIC(const char* str)
{
  Screen::putString("kernel panic:\n");
  Screen::putString(str);
  asm("cli");
  while (true);
}

#define debug(str, i) \
  fDeg() << str << ' ' << std::hex << i;

#endif /* DEBUG_HPP */
