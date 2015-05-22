#include "Symbols.hpp"
#include <cstdint>

extern "C" char Pml4;
char* Symbols::getPml4()
{
  return &Pml4;
}

#define DECLARE_VIRT_SYMBOL(sym, capsym) \
  extern "C" char _##sym; \
  char* Symbols::get##capsym() \
  { \
    return &_##sym; \
  }
#define DECLARE_PHYS_SYMBOL(sym, capsym) \
  extern "C" char _##sym; \
  uintptr_t Symbols::get##capsym() \
  { \
    return reinterpret_cast<uintptr_t>(&_##sym); \
  }

DECLARE_PHYS_SYMBOL(kernelBootstrapStart, KernelBootstrapStart)
DECLARE_PHYS_SYMBOL(kernelTextStart, KernelTextStart)
DECLARE_PHYS_SYMBOL(kernelDataStart, KernelDataStart)

DECLARE_VIRT_SYMBOL(kernelVTextStart, KernelVTextStart)
DECLARE_VIRT_SYMBOL(kernelVTextEnd, KernelVTextEnd);
DECLARE_VIRT_SYMBOL(kernelVDataStart, KernelVDataStart);
DECLARE_VIRT_SYMBOL(kernelVDataEnd, KernelVDataEnd);
DECLARE_VIRT_SYMBOL(kernelVBssEnd, KernelVBssEnd);
DECLARE_VIRT_SYMBOL(stackBase, StackBase);
DECLARE_VIRT_SYMBOL(pageHeapBase, PageHeapBase);
DECLARE_VIRT_SYMBOL(heapBase, HeapBase);
