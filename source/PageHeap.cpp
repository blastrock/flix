#include "PageHeap.hpp"
#include "PageDirectory.hpp"
#include "Util.hpp"
#include "Symbols.hpp"
#include "Memory.hpp"
#include "Debug.hpp"

XLL_LOG_CATEGORY("core/memory/pageheap");

PdPageHeap& getPdPageHeap()
{
  static PdPageHeap pageHeap(Symbols::getPageHeapBase());
  return pageHeap;
}

StackPageHeap& getStackPageHeap()
{
  static StackPageHeap pageHeap(Symbols::getStackPageHeapBase());
  return pageHeap;
}

template <unsigned BSize, unsigned PSize, unsigned SSize>
PageHeap<BSize, PSize, SSize>::PageHeap(char* heapStart)
  : m_heapStart(heapStart)
{
}

template <unsigned BSize, unsigned PSize, unsigned SSize>
std::pair<void*, physaddr_t> PageHeap<BSize, PSize, SSize>::kmalloc()
{
  std::pair<page_index_t, physaddr_t> item = allocBlock();
  return {pageToPtr(item.first), item.second};
}

template <unsigned BSize, unsigned PSize, unsigned SSize>
auto PageHeap<BSize, PSize, SSize>::allocBlock()
    -> std::pair<page_index_t, physaddr_t>
{
  // if we are reentering, use pool
  if (m_allocating)
  {
    assert(PoolSize);

    xDeb("Nested page allocation, taking from pool");
    assert(!m_pool.empty());

    std::pair<page_index_t, physaddr_t> ret = m_pool.back();
    m_pool.resize(m_pool.size() - 1);
    return ret;
  }

  for (unsigned int i = 0; i < m_map.size(); ++i)
    if (!m_map[i])
      return allocPage(i);

  page_index_t index = m_map.size();
  m_map.resize(index + 1);
  return allocPage(index);
}

template <unsigned BSize, unsigned PSize, unsigned SSize>
auto PageHeap<BSize, PSize, SSize>::allocPage(page_index_t index)
    -> std::pair<page_index_t, physaddr_t>
{
  m_allocating = true;

  // assert page is not used yet
  assert(index >= m_map.size() || !m_map[index]);

  // resize vector if page is too far
  if (index >= m_map.size())
  {
    // for now, there should be no reason we ask a page too far appart
    assert(index == m_map.size());
    m_map.resize(index + 1, false);
  }

  m_map[index] = true;

  physaddr_t phys;
  // first StaticSize pages are always mapped
  if (index * BlockSize >= StaticSize)
  {
    PageDirectory::getKernelDirectory()->mapPage(pageToPtr(index),
        PageDirectory::ATTR_RW | PageDirectory::ATTR_NOEXEC,
        &phys);
    for (unsigned n = 1; n < BlockSize; ++n)
      PageDirectory::getKernelDirectory()->mapPage(
          static_cast<char*>(pageToPtr(index)) + n * PAGE_SIZE,
          PageDirectory::ATTR_RW | PageDirectory::ATTR_NOEXEC);
  }
  else
    phys = Symbols::getKernelPageHeapStart() + index * PAGE_SIZE * BlockSize;

  ++m_usedBlockCount;

  m_allocating = false;

  return {index, phys};
}

template <unsigned BSize, unsigned PSize, unsigned SSize>
void PageHeap<BSize, PSize, SSize>::kfree(void* ptr)
{
  if (!ptr)
    return;

  page_index_t index = ptrToPage(ptr);

  assert(index < m_map.size());
  assert(m_map[index]);

  // first StaticSize pages are always mapped
  if (index * BlockSize >= StaticSize)
    for (unsigned n = 0; n < BlockSize; ++n)
    {
      physaddr_t phys = PageDirectory::getKernelDirectory()->unmapPage(
          static_cast<char*>(ptr) + n * PAGE_SIZE);
      Memory::get().setPageFree(phys / PAGE_SIZE);
    }

  --m_usedBlockCount;

  m_map[index] = false;
}

template <unsigned BSize, unsigned PSize, unsigned SSize>
void PageHeap<BSize, PSize, SSize>::refillPool()
{
  // this function is called everytime a page is mapped so it may reenter
  if (m_allocating)
    return;

  // the pool may be used while we refill it
  while (m_pool.size() < PoolSize)
  {
    xDeb("Refilling page pool");
    auto block = allocBlock();
    m_pool.push_back(block);
  }
}

template <unsigned BSize, unsigned PSize, unsigned SSize>
void* PageHeap<BSize, PSize, SSize>::pageToPtr(page_index_t index)
{
  return m_heapStart + index * PAGE_SIZE * BlockSize;
}

template <unsigned BSize, unsigned PSize, unsigned SSize>
auto PageHeap<BSize, PSize, SSize>::ptrToPage(void* ptr) -> page_index_t
{
  return (static_cast<char*>(ptr) - m_heapStart) / PAGE_SIZE / BlockSize;
}

template class PageHeap<2, 16, 32>;
template class PageHeap<4, 0, 0>;
