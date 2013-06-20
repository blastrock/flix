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

  //m_pool.reserve(16);
  //// TODO constants
  //for (unsigned char i = 0; i < 16; ++i)
  //  m_pool.push_back({i, reinterpret_cast<uint8_t*>(0x800000) + i*0x1000});
  //m_map.resize(16, true);
}

std::vector<std::pair<void*, void*>> PageHeap::kmalloc(uint64_t size)
{
  std::vector<std::pair<uint64_t, void*>> items = allocPages(size/0x1000);
  std::vector<std::pair<void*, void*>> ret;
  ret.reserve(items.size());
  std::transform(items.begin(), items.end(), std::back_inserter(ret),
      [](const std::pair<uint64_t, void*>& item) ->
          std::pair<void*, void*> {
        return {pageToPtr(item.first), item.second};
      });
  return ret;
}

std::vector<std::pair<uint64_t, void*>> PageHeap::allocPages(uint64_t nbPages)
{
  std::vector<std::pair<uint64_t, void*>> ret;
  unsigned int lastEmpty = -1;
  // if we are reentering, use pool
  if (m_allocating)
  {
    assert(!m_pool.empty());

    for (unsigned int i = 0; i < m_pool.size(); )
    {
      bool ok = true;
      for (unsigned int j = 0; j < nbPages && i+j < m_pool.size(); ++j)
        // if pages are not contiguous
        if (m_pool[i+j].first != m_pool[i].first + j)
        {
          ok = false;
          break;
        }
        else
          lastEmpty = i+j;

      if (ok)
      {
        for (unsigned int j = 0; j < nbPages && i+j < m_pool.size(); ++j)
          ret.push_back(m_pool[i+j]);
        m_pool.erase(m_pool.begin()+i, m_pool.begin()+i+nbPages);
        return ret;
      }
      else
        i = lastEmpty + 1;
    }

    assert(!"no free page for reentrant allocation");
    return ret;
  }

  for (unsigned int i = 0; i < m_map.size(); )
  {
    bool ok = true;
    for (unsigned int j = 0; j < nbPages && i+j < m_map.size(); ++j)
      if (m_map[i+j])
      {
        ok = false;
        break;
      }
      else
        lastEmpty = i+j;

    if (ok)
    {
      for (unsigned int j = 0; j < nbPages && i+j < m_map.size(); ++j)
        ret.push_back(allocPage(i+j));
      return ret;
    }
    else
      if (lastEmpty == (unsigned int)-1)
        ++i;
      else
        // start next iteration at first potentially unused page
        // lastEmpty is unused, lastEmpty + 1 is used (or is out of range), so we
        // take lastEmpty + 2
        i = lastEmpty + 2;
  }

  if (lastEmpty == (unsigned int)-1)
    lastEmpty = m_map.size();
  m_map.resize(lastEmpty + nbPages);
  for (unsigned int j = lastEmpty; j < m_map.size(); ++j)
    ret.push_back(allocPage(j));
  return ret;
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
  if (index > 32)
    Paging::mapPage(pageToPtr(index), &phys);
  else
    phys = reinterpret_cast<void*>(0xa00000 + index * 0x1000);

  m_allocating = false;

  //refillPool();

  return {index, phys};
}

void* PageHeap::pageToPtr(uint64_t index)
{
  return m_heapStart + index*0x1000;
}

void PageHeap::refillPool()
{
  // the pool may be used while we replenish it
  while (m_pool.size() < 16)
  {
    auto pages = allocPages(2);
    m_pool.insert(m_pool.end(), pages.begin(), pages.end());
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
    Paging::unmapPage(ptr);
  m_map[index] = false;
}
