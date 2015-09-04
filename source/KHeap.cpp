#include "KHeap.hpp"
#include "PageDirectory.hpp"
#include "Util.hpp"
#include "Symbols.hpp"
#include "Debug.hpp"

XLL_LOG_CATEGORY("core/memory/kheap");

static KHeap g_heap;

class KHeap::HeapBlock
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

KHeap& KHeap::get()
{
  return g_heap;
}

void KHeap::init()
{
  uint32_t initialSize = 0x200000;

  m_heapStart = Symbols::getHeapBase();
  m_heapEnd = m_heapStart + initialSize;
  HeapBlock* block = reinterpret_cast<HeapBlock*>(m_heapStart);

  // multiboot header is at heap + 4, treat it as a block
  {
    uint32_t mbSize = *reinterpret_cast<uint32_t*>(m_heapStart + 8);
    mbSize = intAlignSup(mbSize, 4) + 8;
    block->setSize(mbSize);
    block->setUsed(true);

    block = reinterpret_cast<HeapBlock*>(m_heapStart + mbSize);

    initialSize -= mbSize;
  }

  block->setSize(initialSize);
  block->setUsed(false);
  m_lastBlock = block;
}

void* KHeap::kmalloc(uint32_t size)
{
  // count header
  size += 4;
  // ceil to 4 bytes
  size = intAlignSup(size, 4);

  HeapBlock* block;
  char* ptr = m_heapStart;
  while (ptr < m_heapEnd)
  {
    block = reinterpret_cast<HeapBlock*>(ptr);
    uint32_t blockSize = block->getSize();

    assert(blockSize > 8);

    if (!block->getUsed())
    {
      // if block has exact needed size or block is non splittable (minimal
      // block size is 8)
      if (size < blockSize && size >= blockSize - 8)
      {
        block->setUsed(true);
        assert(block->getSize() >= size);
        return block->getData();
      }
      // if block is larger and must be splitted
      else if (size < blockSize)
      {
        std::pair<HeapBlock*, HeapBlock*> blocks = splitBlock(block, size);

        // if this was the last block, there is a new last block
        if (block == m_lastBlock)
          m_lastBlock = blocks.second;

        block = blocks.first;
        block->setUsed(true);

        assert(!blocks.second->getUsed());
        assert(block->getSize() >= size);

        return block->getData();
      }
    }

    ptr += blockSize;
  }

  xDeb("Heap enlarge");

  // count last block size if it's free
  uint32_t blockSize = block->getUsed() ? 0 : block->getSize();
  // asked size - last block size if it's free -> round up
  uint32_t neededPages = (size - blockSize + 0x1000-1) / 0x1000;

  for (uint32_t i = 0; i < neededPages; ++i)
  {
    // kmalloc may be reentered here!
    PageDirectory::getKernelDirectory()->mapPage(m_heapEnd,
        PageDirectory::ATTR_RW);
    m_heapEnd += 0x1000;

    // if block is used, go to next block
    if (!m_lastBlock->getUsed())
      block = m_lastBlock;
    else
    {
      block = ptrAdd(m_lastBlock, m_lastBlock->getSize());

      assert(reinterpret_cast<uint64_t>(block) % 0x1000 == 0);
      assert(reinterpret_cast<char*>(block) < m_heapEnd);

      // set size to 0, it will be updated below
      block->setSize(0);
    }

    block->setSize(block->getSize() + 0x1000);
  }

  assert(size <= block->getSize());

  if (size <= block->getSize() - 8)
    block = splitBlock(block, size).first;

  block->setUsed(true);

  assert(block->getSize() >= size);

  return block->getData();
}

void KHeap::kfree(void* ptr)
{
  if (!ptr)
    return;

  HeapBlock* block = reinterpret_cast<HeapBlock*>(ptrAdd(ptr, -4));
  block->setUsed(false);
  // TODO merge free blocks
}

std::pair<KHeap::HeapBlock*, KHeap::HeapBlock*> KHeap::splitBlock(
    HeapBlock* block, uint64_t size)
{
  assert(!block->getUsed());

  // block is too small to be split!
  assert(size <= block->getSize() - 8);

  uint32_t fullSize = block->getSize();
  HeapBlock* nextBlock = ptrAdd(block, size);

  block->setSize(size);
  nextBlock->setSize(fullSize - size);
  nextBlock->setUsed(false);

  return {block, nextBlock};
}

extern "C"
{

void free(void* ptr)
{
  g_heap.kfree(ptr);
}

void* malloc(size_t size)
{
  return g_heap.kmalloc(size);
}

}
