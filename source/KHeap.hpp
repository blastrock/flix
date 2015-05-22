#ifndef K_HEAP_HPP
#define K_HEAP_HPP

#include <cstdint>
#include <utility>
#include <cassert>

class KHeap
{
  public:
    static KHeap& get();

    void* kmalloc(uint32_t size);
    void kfree(void* ptr);

    void init();

  private:
    class HeapBlock;

    char* m_heapStart;
    char* m_heapEnd;
    HeapBlock* m_lastBlock;

    std::pair<HeapBlock*, HeapBlock*> splitBlock(HeapBlock* block,
        uint64_t size);
};

#endif /* K_HEAP_HPP */
