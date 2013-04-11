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
  m_heapEnd = m_heapStart + 0x200000;
  std::memset(m_heapStart, 0, 0x200000);
}

void* KHeap::kmalloc(uint32_t size)
{
  size += 4;

  uint8_t* ptr = m_heapStart;
  while (ptr < m_heapEnd)
  {
    HeapBlock* block = reinterpret_cast<HeapBlock*>(ptr);
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

  PANIC("not implemented");
  // TODO enlarge heap to allocate block
  return nullptr;
}

void* KHeap::kmalloc_a(uint32_t sz, void** phys)
{
  PANIC("not implemented");
  // TODO
  return nullptr;
}
