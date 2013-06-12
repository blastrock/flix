#include "KHeap.hpp"
#include "Paging.hpp"
#include "Debug.hpp"

// XXX duplicated from Multiboot
template <typename T>
inline T alignSup(T base, uint8_t val)
{
  return (base + val-1) & ~(uint64_t)(val-1);
}

template <typename T>
inline T* ptrAdd(T* ptr, uint64_t val)
{
  return reinterpret_cast<T*>(reinterpret_cast<char*>(ptr)+val);
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
  // count header
  size += 4;
  // ceil to 4 bytes
  size = alignSup(size, 4);

  HeapBlock* block;
  uint8_t* ptr = m_heapStart;
  while (ptr < m_heapEnd)
  {
    block = reinterpret_cast<HeapBlock*>(ptr);
    uint32_t blockSize = block->size & ~0x3;

    if (!(block->size & HEAP_USED))
    {
      // if block has exact needed size or block is non splittable (minimal
      // block size is 8)
      if (size >= blockSize && size < blockSize - 8)
      {
        block->size |= HEAP_USED;
        //assert((block->size & ~0x3) >= size);
        return &block->data;
      }
      // if block is larger and must be splitted
      else if (size < blockSize)
      {
        block = splitBlock(block, size).first;
        block->size |= HEAP_USED;

        //TODO uncomment when assert is implemented
        //assert(!(nextBlock->size & 0x3));
        //assert((block->size & ~0x3) >= size);

        return &block->data;
      }
    }

    ptr += block->size;
  }

  fDeg() << "heap enlarge";

  // count last lock size if it's free
  uint32_t blockSize = block->size & HEAP_USED ? 0 : block->size;
  // asked size - last block size if it's free -> round up
  uint32_t neededPages = (size - blockSize + 0x1000-1) / 0x1000;

  for (uint32_t i = 0; i < neededPages; ++i)
  {
    // check for error!
    Paging::mapPage(m_heapEnd);
    m_heapEnd += 0x1000;
  }

  // if block is used, go to next block
  if (!blockSize)
  {
    block = reinterpret_cast<HeapBlock*>(ptr);
    block->size = 0;
  }

  block->size += neededPages*0x1000;

  block = splitBlock(block, size).first;
  block->size |= HEAP_USED;

  //assert((block->size & ~0x3) >= size);

  return &block->data;
}

std::pair<KHeap::HeapBlock*, KHeap::HeapBlock*> KHeap::splitBlock(
    HeapBlock* block, uint64_t size)
{
  //assert(!(block->size & HEAP_USED));

  if (size > block->size + 8)
  {
    // block is too small to be split!
    // TODO panic is a bit too much, isn't it?
    PANIC("block too small");
  }

  uint32_t fullSize = block->size;
  HeapBlock* nextBlock = ptrAdd(block, size);

  block->size = size;
  nextBlock->size = fullSize - size;

  return {block, nextBlock};
}

void* KHeap::kmalloc_a(uint32_t sz, void** phys)
{
  return kmalloc(sz);
  //PANIC("not implemented");
  // TODO
  return nullptr;
}
