#ifndef PAGE_HEAP_HPP
#define PAGE_HEAP_HPP

#include <cstdint>
#include <vector>
#include <utility>

#include "Types.hpp"

template <unsigned BSize, unsigned PSize, unsigned SSize>
class PageHeap
{
public:
  using page_index_t = uint32_t;

  static constexpr unsigned BlockSize = BSize;  // in pages
  static constexpr unsigned PoolSize = PSize;   // in blocks
  static constexpr unsigned StaticSize = SSize; // in pages

  PageHeap(char* heapStart);

  /**
   * Allocate BSize pages of memory on the page heap
   *
   * \return the virtual address of the allocation and the physical address
   * of the first page.
   */
  std::pair<void*, physaddr_t> kmalloc();
  void kfree(void* ptr);
  void refillPool();
  uint64_t getUsedBlockCount() const
  {
    return m_usedBlockCount;
  }

private:
  uint64_t m_usedBlockCount = 0;

  bool m_allocating = false;
  char* m_heapStart;

  std::vector<bool> m_map;
  std::vector<std::pair<page_index_t, physaddr_t>> m_pool;

  std::pair<page_index_t, physaddr_t> allocBlock();
  std::pair<page_index_t, physaddr_t> allocPage(page_index_t index);

  void* pageToPtr(page_index_t index);
  page_index_t ptrToPage(void* ptr);
};

using PdPageHeap = PageHeap<2, 16, 32>;
using StackPageHeap = PageHeap<4, 0, 0>;

extern template class PageHeap<2, 16, 32>;
extern template class PageHeap<4, 0, 0>;

PdPageHeap& getPdPageHeap();
StackPageHeap& getStackPageHeap();

struct PdAllocator
{
  static PdPageHeap& get()
  {
    return getPdPageHeap();
  }
};

#endif /* PAGE_HEAP_HPP */
