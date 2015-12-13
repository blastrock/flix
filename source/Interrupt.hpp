#ifndef INTERRUPT_HPP
#define INTERRUPT_HPP

#include <cstdint>
#include "TaskManager.hpp"

struct InterruptState
{
  // callee saved
  uint64_t r15, r14, r13, r12, rbx, rbp;
  // caller saved
  uint64_t r11, r10, r9, r8, rax, rcx, rdx, rsi, rdi;
  uint8_t intNo;
  uint64_t errCode;
  uint64_t rip, cs, rflags, rsp, ss;

  Task::Context toTaskContext() const;
} __attribute__((packed));

enum Interrupts
{
  IRQ_TIMER = 0,
  IRQ_KEYBOARD,
};

class Interrupt
{
  public:
    static void initPic();
    static void handle(InterruptState* s);
    static void mask(uint8_t nb);
    static void unmask(uint8_t nb);
};

#endif /* INTERRUPT_HPP */
