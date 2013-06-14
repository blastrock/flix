#include "PageHeap.hpp"
#include "Paging.hpp"
#include "Util.hpp"
#include "Debug.hpp"

extern "C" uint8_t _pageHeapBase;

uint8_t* PageHeap::m_heapStart = nullptr;
bool PageHeap::m_allocating = false;

std::vector<bool> PageHeap::m_map;
std::vector<std::pair<uint64_t, void*>> PageHeap::m_pool;

void PageHeap::init()
{
  m_heapStart = &_pageHeapBase;

  m_pool.reserve(8);
  // TODO constants
  for (unsigned char i = 0; i < 8; ++i)
    m_pool.push_back({i, reinterpret_cast<uint8_t*>(0x800000) + i*0x1000});
  m_map.resize(8, true);
}

std::pair<void*, void*> PageHeap::kmalloc()
{
  std::pair<uint64_t, void*> item = allocPage();
  return {pageToPtr(item.first), item.second};
}

std::pair<uint64_t, void*> PageHeap::allocPage()
{
  for (unsigned int i = 0; i < m_map.size(); ++i)
    if (!m_map[i])
      return allocPage(i);

  return allocPage(m_map.size());
}

std::pair<uint64_t, void*> PageHeap::allocPage(uint64_t index)
{
  // if we are reentering, use pool
  if (m_allocating)
  {
    //assert(!m_pool.empty());

    std::pair<uint64_t, void*> item = m_pool.back();
    m_pool.pop_back();
    return item;
  }

  m_allocating = true;

  // assert page is not used yet
  //assert(m_map.size() <= index || !m_map[index]);

  // resize vector if page is too far
  if (index >= m_map.size())
  {
    // for now, there should be no reason we ask a page too far appart
    //assert(index == m_map.size());
    m_map.resize(index+1, false);
  }

  m_map[index] = true;

  void* phys;
  Paging::mapPage(pageToPtr(index), &phys);

  m_allocating = false;

  refillPool();

  return {index, phys};
}

void* PageHeap::pageToPtr(uint64_t index)
{
  return m_heapStart + index*0x1000;
}

void PageHeap::refillPool()
{
  // the pool may be used while we replenish it
  while (m_pool.size() < 8)
    m_pool.push_back(allocPage());
}

void PageHeap::kfree(void* ptr)
{
  if (!ptr)
    return;

  uint8_t* bptr = static_cast<uint8_t*>(ptr);
  uint64_t index = bptr - m_heapStart;

  //assert(m_map[index]);

  Paging::unmapPage(ptr);
  m_map[index] = false;
}
