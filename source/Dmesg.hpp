#ifndef DMESG_HPP
#define DMESG_HPP

#include <sstream>
#include <cstring>
#include "ConsoleStreamBuf.hpp"
#include "pnt.hpp"

template <typename... Args>
void Degf(const char* fmt, const Args&... args)
{
  ConsoleStreamBuf st;
  pnt::writef(st, fmt, args...);
  st.sputc('\n');
}

#endif /* DMESG_HPP */
