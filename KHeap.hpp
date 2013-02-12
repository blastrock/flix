#ifndef K_HEAP_HPP
#define K_HEAP_HPP

#include "inttypes.hpp"

class KHeap
{
  public:
    static void* kmalloc(u32 sz, void** phys = nullptr);
    static void* kmalloc_a(u32 sz, void** phys = nullptr);

  private:
    static char* placement_address;
};

#endif /* K_HEAP_HPP */
