#ifndef SYSCALL_HPP
#define SYSCALL_HPP

namespace sys
{

inline void print(const char* str)
{
  asm volatile (
      "int $0x80"
      :
      :"a"(str));
}

}

#endif
