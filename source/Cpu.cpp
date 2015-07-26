#include "Cpu.hpp"

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
