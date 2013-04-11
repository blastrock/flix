#ifndef K_HEAP_HPP
#define K_HEAP_HPP

#include <cstdint>

class KHeap
{
  public:
    static void init();

    static void* kmalloc(uint32_t size);
    static void* kmalloc_a(uint32_t sz, void** phys = nullptr);

  private:
    static constexpr uint32_t HEAP_USED = 0x1;

    struct HeapBlock
    {
      uint32_t size;
      uint8_t data;
    } __attribute__((packed));

    static uint8_t* m_heapStart;
    static uint8_t* m_heapEnd;
};

#endif /* K_HEAP_HPP */
