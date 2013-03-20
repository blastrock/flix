#ifndef GENERIC_PAGING_HPP
#define GENERIC_PAGING_HPP

#include <cstdint>
#include <cstring>
#include "KHeap.hpp"
#include "Debug.hpp"

static void* const INVALID_ADDR = reinterpret_cast<void*>(-1L);

inline bool isInvalid(void* ptr)
{
  return ptr == INVALID_ADDR;
}

template <typename T>
inline T* invalidPtr()
{
  return reinterpret_cast<T*>(INVALID_ADDR);
}

class Ppa
{
  public:
    void* allocate(uint64_t size)
    {
      return KHeap::kmalloc_a(size);
    }
};

template <typename CurLevel, typename... Levels>
class PageManager
{
  public:
    typedef CurLevel Entry;
    typedef PageManager<Levels...> NextLayout;
    typedef typename NextLayout::PageType PageType;
    
    static constexpr uint8_t ADD_BITS = CurLevel::ADD_BITS;
    static constexpr uint8_t TOTAL_BITS = ADD_BITS +
      NextLayout::TOTAL_BITS;

    static PageType* getPage(Entry& prevEntry, void* address, bool create);
};

template <typename CurLevel>
class PageManager<CurLevel>
{
  public:
    typedef CurLevel Entry;
    typedef CurLevel PageType;
    
    static constexpr uint8_t ADD_BITS = CurLevel::ADD_BITS;
    static constexpr uint8_t TOTAL_BITS = ADD_BITS;

    static PageType* getPage(Entry& prevEntry, void* address, bool create);
};

template <typename PrevLevel, typename CurLevel, int TOTAL_BITS, int ADD_BITS>
CurLevel* getNextEntry(PrevLevel& prevEntry, void* address, bool create)
{
  CurLevel* directory;
  // if not present
  if (!prevEntry.p)
  {
    if (!create)
      return invalidPtr<CurLevel>();

    // create it
    uint32_t directorySize = sizeof(CurLevel) * (1 << ADD_BITS);
    directory = static_cast<CurLevel*>(KHeap::kmalloc_a(directorySize));
    std::memset(directory, 0, directorySize);
    prevEntry.base = reinterpret_cast<decltype(prevEntry.base)>(directory)
      >> PrevLevel::BASE_SHIFT;
  }
  else
    // get it
    directory = reinterpret_cast<CurLevel*>(
        prevEntry.base << PrevLevel::BASE_SHIFT);

  uint64_t addval = reinterpret_cast<uint64_t>(address);
  uint64_t idx = (addval >> (TOTAL_BITS - ADD_BITS)) & ADD_BITS;

  return &directory[idx];
}

template <typename PrevLevel, typename... Levels>
typename PageManager<PrevLevel, Levels...>::PageType*
  PageManager<PrevLevel, Levels...>::getPage(
    Entry& prevEntry, void* address, bool create)
{
  debug("descending for ", (uint64_t)address);
  debug("in ", (uint64_t)&prevEntry);

  typedef PageManager<PrevLevel, Levels...> PrevPageLayout;
  typedef typename PrevPageLayout::NextLayout CurLayout;
  typedef typename CurLayout::Entry CurLevel;

  CurLevel* entry = getNextEntry<PrevLevel, CurLevel,
    CurLayout::TOTAL_BITS, CurLayout::ADD_BITS>(prevEntry, address, create);

  if (isInvalid(entry))
    return invalidPtr<typename CurLayout::PageType>();

  return PageManager<Levels...>::getPage(*entry, address, create);
}

template <typename PrevLevel>
typename PageManager<PrevLevel>::PageType*
  PageManager<PrevLevel>::getPage(
    Entry& prevEntry, void*, bool)
{
  return &prevEntry;
}

#endif /* GENERIC_PAGING_HPP */
