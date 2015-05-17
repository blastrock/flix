#include "PageHeap.hpp"
#include "PageDirectory.hpp"
#include "Util.hpp"
#include "Debug.hpp"

extern "C" uint8_t _pageHeapBase;

static constexpr unsigned PAGE_SIZE = 0x1000;
static constexpr unsigned BLOCK_SIZE = 2; // in pages

uint8_t* PageHeap::m_heapStart = nullptr;
bool PageHeap::m_allocating = false;

std::vector<bool> PageHeap::m_map;
std::vector<std::pair<uint64_t, void*>> PageHeap::m_pool;

void PageHeap::init()
{
  m_heapStart = &_pageHeapBase;
}

std::pair<void*, void*> PageHeap::kmalloc()
{
  std::pair<uint64_t, void*> item = allocBlock();
  return {pageToPtr(item.first), item.second};
}

std::pair<uint64_t, void*> PageHeap::allocBlock()
{
  // if we are reentering, use pool
  if (m_allocating)
  {
    assert(!m_pool.empty());

    std::pair<uint64_t, void*> ret = m_pool.back();
    m_pool.resize(m_pool.size() - 1);
    return ret;
  }

  for (unsigned int i = 0; i < m_map.size(); ++i)
    if (!m_map[i])
      return allocPage(i);

  uint64_t index = m_map.size();
  m_map.resize(index + 1);
  return allocPage(index);
}

std::pair<uint64_t, void*> PageHeap::allocPage(uint64_t index)
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

  void* phys;
  // first 32 pages are always mapped
  if (index * BLOCK_SIZE > 32)
  {
    PageDirectory::getKernelDirectory()->mapPage(pageToPtr(index), &phys);
    for (unsigned n = 1; n < BLOCK_SIZE; ++n)
      PageDirectory::getKernelDirectory()->mapPage(
          static_cast<char*>(pageToPtr(index)) + n * PAGE_SIZE);
  }
  else
    phys = reinterpret_cast<void*>(0xa00000 + index * PAGE_SIZE * BLOCK_SIZE);

  m_allocating = false;

  //refillPool();

  return {index, phys};
}

void* PageHeap::pageToPtr(uint64_t index)
{
  return m_heapStart + index * PAGE_SIZE * BLOCK_SIZE;
}

void PageHeap::refillPool()
{
  // the pool may be used while we replenish it
  while (m_pool.size() < 16)
  {
    auto block = allocBlock();
    m_pool.push_back(block);
  }
}

void PageHeap::kfree(void* ptr)
{
  if (!ptr)
    return;

  uint8_t* bptr = static_cast<uint8_t*>(ptr);
  uint64_t index = bptr - m_heapStart;

  assert(m_map[index]);

  // first 32 pages are always mapped
  if (index > 32)
    PageDirectory::getKernelDirectory()->unmapPage(ptr);
  m_map[index] = false;
}
