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

class KHeap::HeapBlock
{
  public:
    uint8_t* getData()
    {
      assert(&data - reinterpret_cast<uint8_t*>(this) == HEADER_SIZE &&
          "HEADER_SIZE is incorrect");
      return &data;
    }

    uint32_t getSize() const
    {
      return state.sizeUpper << BLOCK_ALIGN_SHIFT;
    }
    void setSize(uint32_t asize)
    {
      assert(asize % BLOCK_ALIGN == 0);
      state.sizeUpper = asize >> BLOCK_ALIGN_SHIFT;
    }

    bool getUsed() const
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
  static KHeap g_heap;
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
  xDeb("kmalloc(%d)", size);
  if (!size)
    return nullptr;

  auto lock = m_mutex.getScoped();

  // count header
  size += HEADER_SIZE;
  // ceil to align
  size = intAlignSup(size, BLOCK_ALIGN);

  HeapBlock* block = nullptr;
  char* ptr = m_heapStart;
  while (ptr < m_heapEnd)
  {
    block = reinterpret_cast<HeapBlock*>(ptr);
    uint32_t blockSize = block->getSize();

    assert(block <= m_lastBlock);
    assert(blockSize >= BLOCK_MIN_SIZE);

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
        assert(blocks.second->getSize() >= BLOCK_MIN_SIZE);
        assert(block->getSize() >= size);

        return block->getData();
      }
    }

    ptr += blockSize;
  }

  xDeb("Heap enlarge");

  // count last block size if it's free
  assert(block == m_lastBlock);
  const uint32_t blockSize = block->getUsed() ? 0 : block->getSize();
  // asked size - last block size if it's free -> round up
  const uint32_t neededPages = (size - blockSize + 0x1000-1) / 0x1000;

  enlargeHeap(neededPages);

  block = m_lastBlock;

  assert(size <= block->getSize());

  // split the block if it's too large
  if (size <= block->getSize() - BLOCK_MIN_SIZE)
  {
    const auto blocks = splitBlock(block, size);
    block = blocks.first;
    m_lastBlock = blocks.second;
  }

  block->setUsed(true);

  assert(block->getSize() >= size);
  assert(reinterpret_cast<char*>(m_lastBlock) + m_lastBlock->getSize() ==
         m_heapEnd);

  return block->getData();
}

void KHeap::enlargeHeap(const std::size_t pageCount)
{
  for (uint32_t i = 0; i < pageCount; ++i)
  {
    // FIXME kmalloc may be reentered here because of Memory, Memory should
    // allocate its vector on another heap
    PageDirectory::getKernelDirectory()->mapPage(m_heapEnd,
        PageDirectory::ATTR_RW | PageDirectory::ATTR_NOEXEC);
    m_heapEnd += PAGE_SIZE;

    HeapBlock* block;
    if (m_lastBlock->getUsed())
    {
      // if last block is used, create a new block
      block = ptrAdd(m_lastBlock, m_lastBlock->getSize());

      assert(reinterpret_cast<uint64_t>(block) % PAGE_SIZE == 0);
      assert(reinterpret_cast<char*>(block) < m_heapEnd);

      // set size to 0, it will be updated below
      block->setSize(0);

      m_lastBlock = block;
    }
    else
      // else enlarge it
      block = m_lastBlock;

    block->setSize(block->getSize() + PAGE_SIZE);
  }

  assert(reinterpret_cast<char*>(m_lastBlock) + m_lastBlock->getSize() ==
         m_heapEnd);
}

void KHeap::kfree(void* ptr)
{
  if (!ptr)
    return;

  auto lock = m_mutex.getScoped();

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
  KHeap::get().kfree(ptr);
}

void* malloc(size_t size)
{
  return KHeap::get().kmalloc(size);
}

}
