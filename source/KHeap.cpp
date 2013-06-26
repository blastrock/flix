#include "KHeap.hpp"
#include "Paging.hpp"
#include "Util.hpp"
#include "Symbols.hpp"
#include "Debug.hpp"

uint8_t* KHeap::m_heapStart;
uint8_t* KHeap::m_heapEnd;
KHeap::HeapBlock* KHeap::m_lastBlock;

void KHeap::init()
{
  uint32_t initialSize = 0x200000;

  m_heapStart = static_cast<uint8_t*>(Symbols::getHeapBase());
  m_heapEnd = m_heapStart + initialSize;
  HeapBlock* block = reinterpret_cast<HeapBlock*>(m_heapStart);

  // multiboot header is at heap + 4, treat it as a block
  {
    uint32_t mbSize = *reinterpret_cast<uint32_t*>(m_heapStart + 8);
    mbSize = intAlignSup(mbSize, 4) + 8;
    block->size = mbSize;
    block->state.used = true;

    block = reinterpret_cast<HeapBlock*>(m_heapStart + mbSize);

    initialSize -= mbSize;
  }

  block->size = initialSize;
  m_lastBlock = block;
}

void* KHeap::kmalloc(uint32_t size)
{
  // count header
  size += 4;
  // ceil to 4 bytes
  size = intAlignSup(size, 4);

  HeapBlock* block;
  uint8_t* ptr = m_heapStart;
  while (ptr < m_heapEnd)
  {
    block = reinterpret_cast<HeapBlock*>(ptr);
    uint32_t blockSize = block->size & ~0x3;

    assert(blockSize > 8);

    if (!block->state.used)
    {
      // if block has exact needed size or block is non splittable (minimal
      // block size is 8)
      if (size < blockSize && size >= blockSize - 8)
      {
        block->state.used = true;
        assert((block->size & ~0x3) >= size);
        return &block->data;
      }
      // if block is larger and must be splitted
      else if (size < blockSize)
      {
        std::pair<HeapBlock*, HeapBlock*> blocks = splitBlock(block, size);

        // if this was the last block, there is a new last block
        if (block == m_lastBlock)
          m_lastBlock = blocks.second;

        block = blocks.first;
        block->state.used = true;

        assert(!(blocks.second->size & 0x3));
        assert((block->size & ~0x3) >= size);

        return &block->data;
      }
    }

    ptr += blockSize;
  }

  fDeg() << "heap enlarge";

  // count last lock size if it's free
  uint32_t blockSize = block->state.used ? 0 : block->size;
  // asked size - last block size if it's free -> round up
  uint32_t neededPages = (size - blockSize + 0x1000-1) / 0x1000;

  for (uint32_t i = 0; i < neededPages; ++i)
  {
    // kmalloc may be reentered here!
    Paging::mapPage(m_heapEnd);
    m_heapEnd += 0x1000;

    // if block is used, go to next block
    if (!m_lastBlock->state.used)
      block = m_lastBlock;
    else
    {
      block = ptrAdd(m_lastBlock, m_lastBlock->size & ~0x3);

      assert(reinterpret_cast<uint64_t>(block) % 0x1000 == 0);
      assert(reinterpret_cast<uint8_t*>(block) < m_heapEnd);

      // set size to 0, it will be updated below
      block->size = 0;
    }

    block->size += 0x1000;
  }

  assert(size <= block->size);

  if (size <= block->size - 8)
    block = splitBlock(block, size).first;

  block->state.used = true;

  assert((block->size & ~0x3) >= size);

  return &block->data;
}

void KHeap::kfree(void* ptr)
{
  if (!ptr)
    return;

  HeapBlock* block = reinterpret_cast<HeapBlock*>(ptrAdd(ptr, -4));
  block->state.used = false;
  // TODO merge free blocks
}

std::pair<KHeap::HeapBlock*, KHeap::HeapBlock*> KHeap::splitBlock(
    HeapBlock* block, uint64_t size)
{
  assert(!(block->size & HEAP_USED));

  // block is too small to be split!
  assert(size <= block->size - 8);

  uint32_t fullSize = block->size;
  HeapBlock* nextBlock = ptrAdd(block, size);

  block->size = size;
  nextBlock->size = fullSize - size;

  return {block, nextBlock};
}

namespace std_impl
{
  void free(void* ptr)
  {
    KHeap::kfree(ptr);
  }

  void* malloc(size_t size)
  {
    return KHeap::kmalloc(size);
  }
}
