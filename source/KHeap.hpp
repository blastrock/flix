#ifndef K_HEAP_HPP
#define K_HEAP_HPP

#include <cstdint>
#include <utility>
#include <cassert>

class KHeap
{
  public:
    static void init();

    static void* kmalloc(uint32_t size);
    static void kfree(void* ptr);

  private:
    static constexpr uint32_t HEAP_USED = 0x1;

    class HeapBlock
    {
      public:
        uint8_t* getData()
        {
          return &data;
        }

        uint32_t getSize()
        {
          return state.sizeUpper << 2;
        }
        void setSize(uint32_t asize)
        {
          assert(asize % 4 == 0);
          state.sizeUpper = asize >> 2;
        }

        bool getUsed()
        {
          return state.used;
        }
        void setUsed(bool used)
        {
          state.used = used;
        }

      private:
        struct State
        {
          unsigned used : 1;
          unsigned avl : 1;
          unsigned sizeUpper : 30;
        };

        union
        {
          State state;
          uint32_t size;
        };
        uint8_t data;
    } __attribute__((packed));

    static uint8_t* m_heapStart;
    static uint8_t* m_heapEnd;
    static HeapBlock* m_lastBlock;

    static std::pair<HeapBlock*, HeapBlock*> splitBlock(HeapBlock* block,
        uint64_t size);
};

#endif /* K_HEAP_HPP */
