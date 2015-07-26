#ifndef CPU_HPP
#define CPU_HPP

#include <cstdint>

namespace Cpu
{

uint64_t rflags();
void setKernelStack(void* stack);

}

#endif /* CPU_HPP */
