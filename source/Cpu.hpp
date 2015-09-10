#ifndef CPU_HPP
#define CPU_HPP

#include <cstdint>

namespace Cpu
{

static constexpr uint32_t MSR_EFER = 0xC0000080;
static constexpr uint32_t MSR_STAR = 0xC0000081;
static constexpr uint32_t MSR_LSTAR = 0xC0000082;

uint64_t rflags();

inline void writeMsr(uint32_t msr, uint64_t value)
{
  asm volatile ("wrmsr"
      :
      :"c"(msr)
      ,"d"(static_cast<uint32_t>(value >> 32))
      ,"a"(static_cast<uint32_t>(value))
     );
}

inline uint64_t readMsr(uint32_t msr)
{
  uint32_t low, high;
  asm volatile (
      "rdmsr\n"
      "btsl $0, %%eax\n"
      "wrmsr\n"
      :"=a"(low), "=d"(high)
      :"c"(msr)
      );
  return static_cast<uint64_t>(high) << 32 | low;
}

void setKernelStack(void* stack);

}

#endif /* CPU_HPP */
