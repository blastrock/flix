#ifndef SYMBOLS_HPP
#define SYMBOLS_HPP

class Symbols
{
  public:
    static void* getPml4();
    static void* getKernelBootstrapStart();
    static void* getKernelBssEnd();
    static void* getVirtualBase();
    static void* getHeapBase();
};

#endif /* SYMBOLS_HPP */
