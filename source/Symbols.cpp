#include "Symbols.hpp"
#include <cstdint>

extern "C" uint8_t Pml4;
extern "C" uint8_t _kernelBootstrapStart;
extern "C" uint8_t _kernelTextStart;
extern "C" uint8_t _kernelVTextStart;
extern "C" uint8_t _kernelVTextEnd;
extern "C" uint8_t _kernelVBssEnd;
extern "C" uint8_t VIRTUAL_BASE;
extern "C" uint8_t _stackBase;
extern "C" uint8_t _pageHeapBase;
extern "C" uint8_t _heapBase;

void* Symbols::getPml4()
{
  return &Pml4;
}

uintptr_t Symbols::getKernelBootstrapStart()
{
  return reinterpret_cast<uintptr_t>(&_kernelBootstrapStart);
}

uintptr_t Symbols::getKernelTextStart()
{
  return reinterpret_cast<uintptr_t>(&_kernelTextStart);
}

void* Symbols::getKernelVTextStart()
{
  return &_kernelVTextStart;
}

void* Symbols::getKernelVTextEnd()
{
  return &_kernelVTextStart;
}

void* Symbols::getKernelVBssEnd()
{
  return &_kernelVBssEnd;
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
