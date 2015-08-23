#include "Cpu.hpp"

// this variable is read from the syscall asm code to know to what stack switch
// to on syscall fast path
void* kernelStack;

uint64_t Cpu::rflags()
{
  uint64_t ret;
  asm volatile (
      "pushf\n"
      "popq %0\n"
      : "=r"(ret));
  return ret;
}

void Cpu::setKernelStack(void* stack)
{
  kernelStack = stack;
}
