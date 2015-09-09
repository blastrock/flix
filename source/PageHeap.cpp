#include "PageHeap.hpp"
#include "PageDirectory.hpp"
#include "Util.hpp"
#include "Symbols.hpp"
#include "Memory.hpp"
#include "Debug.hpp"

XLL_LOG_CATEGORY("core/memory/pageheap");

static constexpr unsigned BLOCK_SIZE = 2; // in pages

static PageHeap g_pageHeap;

PageHeap& PageHeap::get()
{
  return g_pageHeap;
}

void PageHeap::init()
{
  m_heapStart = Symbols::getPageHeapBase();
  m_allocating = false;
}

std::pair<void*, physaddr_t> PageHeap::kmalloc()
{
  std::pair<page_index_t, physaddr_t> item = allocBlock();
  return {pageToPtr(item.first), item.second};
}

std::pair<PageHeap::page_index_t, physaddr_t> PageHeap::allocBlock()
{
  // if we are reentering, use pool
  if (m_allocating)
  {
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

std::pair<PageHeap::page_index_t, physaddr_t>
  PageHeap::allocPage(page_index_t index)
{
  m_allocating = true;

  // assert page is not used yet
  assert(index >= m_map.size() || !m_map[index]);

  // resize vector if page is too far
  if (index >= m_map.size())
  {
    // for now, there should be no reason we ask a page too far appart
    assert(index == m_map.size());
    m_map.resize(index+1, false);
  }

  m_map[index] = true;

  physaddr_t phys;
  // first 32 pages are always mapped
  if (index * BLOCK_SIZE >= 32)
  {
    PageDirectory::getKernelDirectory()->mapPage(pageToPtr(index),
        PageDirectory::ATTR_RW, &phys);
    for (unsigned n = 1; n < BLOCK_SIZE; ++n)
      PageDirectory::getKernelDirectory()->mapPage(
          static_cast<char*>(pageToPtr(index)) + n * PAGE_SIZE,
          PageDirectory::ATTR_RW);
  }
  else
    phys = Symbols::getKernelPageHeapStart() + index * PAGE_SIZE * BLOCK_SIZE;

  m_allocating = false;

  return {index, phys};
}

void* PageHeap::pageToPtr(page_index_t index)
{
  return m_heapStart + index * PAGE_SIZE * BLOCK_SIZE;
}

PageHeap::page_index_t PageHeap::ptrToPage(void* ptr)
{
  return (static_cast<char*>(ptr) - m_heapStart) / PAGE_SIZE / BLOCK_SIZE;
}

void PageHeap::refillPool()
{
  // this function is called everytime a page is mapped so it may reenter
  if (m_allocating)
    return;

  // the pool may be used while we refill it
  while (m_pool.size() < 16)
  {
    xDeb("Refilling page pool");
    auto block = allocBlock();
    m_pool.push_back(block);
  }
}

void PageHeap::kfree(void* ptr)
{
  if (!ptr)
    return;

  page_index_t index = ptrToPage(ptr);

  assert(m_map[index]);

  // first 32 pages are always mapped
  if (index * BLOCK_SIZE >= 32)
  {
    physaddr_t phys = PageDirectory::getKernelDirectory()->unmapPage(ptr);
    Memory::setPageFree(phys / PAGE_SIZE);
    for (unsigned n = 1; n < BLOCK_SIZE; ++n)
    {
      phys = PageDirectory::getKernelDirectory()->unmapPage(
          static_cast<char*>(ptr) + n * PAGE_SIZE);
      Memory::setPageFree(phys / PAGE_SIZE);
    }
  }
  m_map[index] = false;
}
