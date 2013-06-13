#ifndef SYMBOLS_HPP
#define SYMBOLS_HPP

class Symbols
{
  public:
    static void* getPml4();
    static void* getKernelStart();
    static void* getKernelBssEnd();
    static void* getVirtualBase();
};

#endif /* SYMBOLS_HPP */
