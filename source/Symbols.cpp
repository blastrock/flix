#include "Symbols.hpp"
#include <cstdint>

extern "C" uint8_t Pml4;
extern "C" uint8_t _kernelBootstrapStart;
extern "C" uint8_t _kernelBssEnd;
extern "C" uint8_t VIRTUAL_BASE;
extern "C" uint8_t _heapBase;

void* Symbols::getPml4()
{
  return &Pml4;
}

void* Symbols::getKernelBootstrapStart()
{
  return &_kernelBootstrapStart;
}

void* Symbols::getKernelBssEnd()
{
  return &_kernelBssEnd;
}

void* Symbols::getVirtualBase()
{
  return &VIRTUAL_BASE;
}

void* Symbols::getHeapBase()
{
  return &_heapBase;
}
