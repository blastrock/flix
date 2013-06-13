#include "Symbols.hpp"
#include <cstdint>

extern "C" uint8_t Pml4;
extern "C" uint8_t _kernelStart;
extern "C" uint8_t _kernelBssEnd;
extern "C" uint8_t VIRTUAL_BASE;

void* Symbols::getPml4()
{
  return &Pml4;
}

void* Symbols::getKernelStart()
{
  return &_kernelStart;
}

void* Symbols::getKernelBssEnd()
{
  return &_kernelBssEnd;
}

void* Symbols::getVirtualBase()
{
  return &VIRTUAL_BASE;
}
