#ifndef K_HEAP_HPP
#define K_HEAP_HPP

#include <cstdint>
#include <utility>

class KHeap
{
  public:
    static void init();

    static void* kmalloc(uint32_t size);

  private:
    static constexpr uint32_t HEAP_USED = 0x1;

    struct HeapBlock
    {
      uint32_t size;
      uint8_t data;
    } __attribute__((packed));

    static uint8_t* m_heapStart;
    static uint8_t* m_heapEnd;

    static std::pair<HeapBlock*, HeapBlock*> splitBlock(HeapBlock* block,
        uint64_t size);
};

#endif /* K_HEAP_HPP */
