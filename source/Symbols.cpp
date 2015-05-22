#include "Symbols.hpp"
#include <cstdint>

extern "C" char Pml4;
extern "C" char _kernelBootstrapStart;
extern "C" char _kernelTextStart;
extern "C" char _kernelVTextStart;
extern "C" char _kernelVTextEnd;
extern "C" char _kernelVBssEnd;
extern "C" char VIRTUAL_BASE;
extern "C" char _stackBase;
extern "C" char _pageHeapBase;
extern "C" char _heapBase;

char* Symbols::getPml4()
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

char* Symbols::getKernelVTextStart()
{
  return &_kernelVTextStart;
}

char* Symbols::getKernelVTextEnd()
{
  return &_kernelVTextStart;
}

char* Symbols::getKernelVBssEnd()
{
  return &_kernelVBssEnd;
}

char* Symbols::getVirtualBase()
{
  return &VIRTUAL_BASE;
}

char* Symbols::getStackBase()
{
  return &_stackBase;
}

char* Symbols::getPageHeapBase()
{
  return &_pageHeapBase;
}

char* Symbols::getHeapBase()
{
  return &_heapBase;
}
