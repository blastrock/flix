#ifndef SYSCALL_HPP
#define SYSCALL_HPP

namespace sys
{

enum ScId
{
  exit = 1
};

inline void call(ScId sc)
{
  asm volatile (
      "int $0x80"
      :
      :"a"(static_cast<uint64_t>(sc)));
}

}

#endif
