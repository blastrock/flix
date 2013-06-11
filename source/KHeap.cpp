#include "KHeap.hpp"
#include "Debug.hpp"

// XXX duplicated from Multiboot
template <typename T>
inline T alignSup(T base, uint8_t val)
{
  return (base + val-1) & ~(uint64_t)(val-1);
}

extern "C" uint8_t _heapBase;

uint8_t* KHeap::m_heapStart;
uint8_t* KHeap::m_heapEnd;

void KHeap::init()
{
  m_heapStart = &_heapBase;
  m_heapEnd = m_heapStart + 0x1000000;
  HeapBlock* block = reinterpret_cast<HeapBlock*>(m_heapStart);
  block->size = 0x1000000;
}

void* KHeap::kmalloc(uint32_t size)
{
  size += 4;

  HeapBlock* block;
  uint8_t* ptr = m_heapStart;
  while (ptr < m_heapEnd)
  {
    block = reinterpret_cast<HeapBlock*>(ptr);
    uint32_t blockSize = block->size & ~0x3;

    if (!(block->size & HEAP_USED))
    {
      if (size == blockSize)
      {
        block->size |= HEAP_USED;
        return &block->data;
      }
      else if (size <= blockSize - 8)
      {
        uint32_t firstBlockSize = alignSup(size, 4);
        uint32_t fullSize = blockSize;
        HeapBlock* nextBlock = reinterpret_cast<HeapBlock*>(
            ptr + firstBlockSize);

        block->size = firstBlockSize | HEAP_USED;
        nextBlock->size = fullSize - size;

        //TODO uncomment when assert is implemented
        //assert(!(nextBlock->size & 0x3));

        return &block->data;
      }
    }

    ptr += block->size;
  }

  fDeg() << "heap enlarge";

  //uint32_t blockSize = block->size;
  //uint32_t neededPages = (size - blockSize + 0x1000-1) / 0x1000;

  //Paging::mapPage(m_heapEnd);

  return nullptr;
}

void* KHeap::kmalloc_a(uint32_t sz, void** phys)
{
  return kmalloc(sz);
  //PANIC("not implemented");
  // TODO
  return nullptr;
}
