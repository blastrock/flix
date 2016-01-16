#ifndef SYMBOLS_HPP
#define SYMBOLS_HPP

#include <stdint.h>

class Symbols
{
public:
  static char* getPml4();
  static uintptr_t getKernelBootstrapStart();
  static uintptr_t getKernelTextStart();
  static uintptr_t getKernelRodataStart();
  static uintptr_t getKernelDataStart();
  static uintptr_t getKernelBssEnd();
  static uintptr_t getKernelPageHeapStart();
  static char* getKernelVBase()
  {
    return getKernelVTextStart();
  }
  static char* getKernelVTextStart();
  static char* getKernelVTextEnd();
  static char* getKernelVRodataStart();
  static char* getKernelVRodataEnd();
  static char* getKernelVDataStart();
  static char* getKernelVDataEnd();
  static char* getKernelVBssEnd();
  static char* getKernelVVga();
  static char* getStackBase();
  static char* getPageHeapBase();
  static char* getStackPageHeapBase();
  static char* getHeapBase();
};

#endif /* SYMBOLS_HPP */
