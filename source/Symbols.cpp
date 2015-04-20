#include "Symbols.hpp"
#include <cstdint>

extern "C" uint8_t Pml4;
extern "C" uint8_t _kernelBootstrapStart;
extern "C" uint8_t _kernelTextStart;
extern "C" uint8_t _kernelVTextStart;
extern "C" uint8_t _kernelBssEnd;
extern "C" uint8_t VIRTUAL_BASE;
extern "C" uint8_t _stackBase;
extern "C" uint8_t _pageHeapBase;
extern "C" uint8_t _heapBase;

void* Symbols::getPml4()
{
  return &Pml4;
}

void* Symbols::getKernelBootstrapStart()
{
  return &_kernelBootstrapStart;
}

void* Symbols::getKernelTextStart()
{
  return &_kernelTextStart;
}

void* Symbols::getKernelVTextStart()
{
  return &_kernelVTextStart;
}

void* Symbols::getKernelBssEnd()
{
  return &_kernelBssEnd;
}

void* Symbols::getVirtualBase()
{
  return &VIRTUAL_BASE;
}

void* Symbols::getStackBase()
{
  return &_stackBase;
}

void* Symbols::getPageHeapBase()
{
  return &_pageHeapBase;
}

void* Symbols::getHeapBase()
{
  return &_heapBase;
}
