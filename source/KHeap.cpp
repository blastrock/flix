#include "KHeap.hpp"
#include "PageDirectory.hpp"
#include "Util.hpp"
#include "Symbols.hpp"
#include "Debug.hpp"

XLL_LOG_CATEGORY("core/memory/kheap");

static constexpr unsigned HEADER_SIZE = 4;
static constexpr unsigned BLOCK_ALIGN_SHIFT = 2;
static constexpr unsigned BLOCK_ALIGN = 1 << BLOCK_ALIGN_SHIFT;
static constexpr unsigned BLOCK_MIN_SIZE = HEADER_SIZE + 4;

static KHeap g_heap;

class KHeap::HeapBlock
{
  public:
    uint8_t* getData()
    {
      assert(&data - reinterpret_cast<uint8_t*>(this) == HEADER_SIZE &&
          "HEADER_SIZE is incorrect");
      return &data;
    }

    uint32_t getSize()
    {
      return state.sizeUpper << BLOCK_ALIGN_SHIFT;
    }
    void setSize(uint32_t asize)
    {
      assert(asize % BLOCK_ALIGN == 0);
      state.sizeUpper = asize >> BLOCK_ALIGN_SHIFT;
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

    static_assert(sizeof(State) == 4, "sizeof(State) != 4");

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

  // multiboot header is at heap + 4 aligned to 8, treat it as a block
  {
    const uint32_t mbSize =
      *reinterpret_cast<uint32_t*>(ptrAlignSup(m_heapStart + HEADER_SIZE, 8));
    const uint32_t blockSize =
      intAlignSup(mbSize, BLOCK_ALIGN) + HEADER_SIZE;
    xDeb("Multiboot header size is %d bytes", mbSize);
    assert(blockSize > BLOCK_MIN_SIZE);
    block->setSize(blockSize);
    block->setUsed(true);

    block = reinterpret_cast<HeapBlock*>(m_heapStart + blockSize);

    initialSize -= blockSize;
  }

  block->setSize(initialSize);
  block->setUsed(false);
  m_lastBlock = block;
}

void* KHeap::kmalloc(uint32_t size)
{
  if (!size)
    return nullptr;

  // count header
  size += HEADER_SIZE;
  // ceil to align
  size = intAlignSup(size, BLOCK_ALIGN);

  HeapBlock* block;
  char* ptr = m_heapStart;
  while (ptr < m_heapEnd)
  {
    block = reinterpret_cast<HeapBlock*>(ptr);
    uint32_t blockSize = block->getSize();

    assert(blockSize > BLOCK_MIN_SIZE);

    if (!block->getUsed())
    {
      // if block has exact needed size or block is non splittable
      if (size < blockSize && size >= blockSize - BLOCK_MIN_SIZE)
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
        PageDirectory::ATTR_RW | PageDirectory::ATTR_NOEXEC);
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

  if (size <= block->getSize() - BLOCK_MIN_SIZE)
    block = splitBlock(block, size).first;

  block->setUsed(true);

  assert(block->getSize() >= size);

  return block->getData();
}

void KHeap::kfree(void* ptr)
{
  if (!ptr)
    return;

  HeapBlock* block =
    reinterpret_cast<HeapBlock*>(ptrAdd(ptr, -(int)HEADER_SIZE));
  block->setUsed(false);
  // TODO merge free blocks
}

std::pair<KHeap::HeapBlock*, KHeap::HeapBlock*> KHeap::splitBlock(
    HeapBlock* block, uint64_t size)
{
  assert(!block->getUsed());

  // block is too small to be split!
  assert(size <= block->getSize() - BLOCK_MIN_SIZE);

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
