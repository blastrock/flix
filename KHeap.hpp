#ifndef K_HEAP_HPP
#define K_HEAP_HPP

#include <cstdint>

class KHeap
{
  public:
    static void* kmalloc(uint32_t sz, void** phys = nullptr);
    static void* kmalloc_a(uint32_t sz, void** phys = nullptr);

  private:
    static char* placement_address;
};

#endif /* K_HEAP_HPP */
