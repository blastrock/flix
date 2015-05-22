#ifndef SYMBOLS_HPP
#define SYMBOLS_HPP

#include <stdint.h>

class Symbols
{
  public:
    static void* getPml4();
    static uintptr_t getKernelBootstrapStart();
    static uintptr_t getKernelTextStart();
    static void* getKernelVTextStart();
    static void* getKernelVTextEnd();
    static void* getKernelVBssEnd();
    static void* getVirtualBase();
    static void* getStackBase();
    static void* getPageHeapBase();
    static void* getHeapBase();
};

#endif /* SYMBOLS_HPP */
