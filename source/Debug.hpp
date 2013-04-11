#ifndef DEBUG_HPP
#define DEBUG_HPP

#include "Screen.hpp"

inline void PANIC(const char* str)
{
  Screen::putString("kernel panic:\n");
  Screen::putString(str);
  while (true);
}

inline void debug(const char* str, long val)
{
  Screen::putString(str);
  Screen::putString(" ");
  Screen::putHex(val);
  Screen::putString("\n");
}

#endif /* DEBUG_HPP */
