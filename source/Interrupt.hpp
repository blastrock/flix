#ifndef INTERRUPT_HPP
#define INTERRUPT_HPP

#include <cstdint>

struct InterruptState
{
  uint64_t r11, r10, r9, r8, rax, rcx, rdx, rsi, rdi;
  uint8_t intNo;
  uint64_t errCode;
  uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed));

class InterruptHandler
{
  public:
    static void handle(InterruptState* s);
};

#endif /* INTERRUPT_HPP */
