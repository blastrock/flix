#ifndef DEBUG_HPP
#define DEBUG_HPP

#include "io.hpp"
#include <xll/log.hpp>

#define dumpRegisters() \
{ \
  uint64_t r15, r14, r13, r12, rbx, rbp; \
  uint64_t r11, r10, r9, r8, rax, rcx, rdx, rsi, rdi; \
  uint64_t rflags, rsp; \
  asm volatile ( \
      "mov %%r15, %0\n" \
      "mov %%r14, %1\n" \
      "mov %%r13, %2\n" \
      "mov %%r12, %3\n" \
      "mov %%rbx, %4\n" \
      "mov %%rbp, %5\n" \
      "mov %%r11, %6\n" \
      "mov %%r10, %7\n" \
      "mov %%r9, %8\n" \
      "mov %%r8, %9\n" \
      "mov %%rax, %10\n" \
      "mov %%rcx, %11\n" \
      "mov %%rdx, %12\n" \
      "mov %%rsi, %13\n" \
      "mov %%rdi, %14\n" \
      "pushf\n" \
      "popq %%rax\n" \
      "mov %%rax, %15\n" \
      "mov %%rsp, %16\n" \
      : "=m"(r15) \
      , "=m"(r14) \
      , "=m"(r13) \
      , "=m"(r12) \
      , "=m"(rbx) \
      , "=m"(rbp) \
      , "=m"(r11) \
      , "=m"(r10) \
      , "=m"(r9) \
      , "=m"(r8) \
      , "=m"(rax) \
      , "=m"(rcx) \
      , "=m"(rdx) \
      , "=m"(rsi) \
      , "=m"(rdi) \
      , "=m"(rflags) \
      , "=m"(rsp) \
      : \
      : "rax" \
      ); \
  xDeb("r15 = %x\nr14 = %x\nr13 = %x\nr12 = %x\nrbx = %x\nrbp = %x\nr11 = %x\nr10 = %x\nr9 = %x\nr8 = %x\nrax = %x\nrcx = %x\nrdx = %x\nrsi = %x\nrdi = %x\nrflags = %x\nrsp = %x\n", r15, r14, r13, r12, rbx, rbp, r11, r10, r9, r8, rax, rcx, rdx, rsi, rdi, rflags, rsp); \
}

#define dumpContextRegisters(context) \
{ \
  xDeb("r15 = %x\nr14 = %x\nr13 = %x\nr12 = %x\nrbx = %x\nrbp = %x\nr11 = %x\nr10 = %x\nr9 = %x\nr8 = %x\nrax = %x\nrcx = %x\nrdx = %x\nrsi = %x\nrdi = %x\nrip = %x\ncs = %x\nrflags = %x\nrsp = %x\nss = %x\n", context.r15, context.r14, context.r13, context.r12, context.rbx, context.rbp, context.r11, context.r10, context.r9, context.r8, context.rax, context.rcx, context.rdx, context.rsi, context.rdi, context.rip, context.cs, context.rflags, context.rsp, context.ss); \
}

#define DBG_BREAK() \
  asm volatile ("xchgw %bx, %bx")

[[noreturn]] inline void PANIC(const char* str)
{
  asm volatile ("cli");
  xFatC("PANIC", "Kernel panic: %s", str);
  DBG_BREAK();
  while (true)
    asm volatile ("hlt");
}

void printE9(const char* str);

void printStackTrace();
void printStackTrace(uint64_t stackPointer);

#endif /* DEBUG_HPP */
