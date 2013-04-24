#ifndef GENERIC_PAGING_HPP
#define GENERIC_PAGING_HPP

#include <cstdint>
#include <cstring>
#include <new>
#include "Debug.hpp"

static void* const INVALID_ADDR = reinterpret_cast<void*>(-1L);

inline bool isInvalid(void* ptr)
{
  return ptr == INVALID_ADDR;
}

template <typename T>
inline T* invalidPtr()
{
  return static_cast<T*>(INVALID_ADDR);
}

template <typename T0, typename... T>
struct LastType
{
  typedef typename LastType<T...>::type type;
};

template <typename T0>
struct LastType<T0>
{
  typedef T0 type;
};

template <typename Allocator, typename CurLevel, typename... Levels>
class PageManager
{
  public:
    typedef CurLevel Entry;
    typedef PageManager<Allocator, Levels...> NextLayout;
    typedef typename LastType<CurLevel, Levels...>::type PageType;

    static constexpr uint8_t ADD_BITS = CurLevel::ADD_BITS;
    static constexpr uint8_t TOTAL_BITS = ADD_BITS + NextLayout::TOTAL_BITS;

    PageManager();

    PageType* getPage(uint64_t address, bool create, Allocator& allocator);
    Entry* getDirectory();

  private:
    Entry m_entries[1 << ADD_BITS];
    NextLayout* m_nextLayouts[1 << ADD_BITS];
};

template <typename Allocator, typename CurLevel>
class PageManager<Allocator, CurLevel>
{
  public:
    typedef CurLevel Entry;
    typedef CurLevel PageType;

    static constexpr uint8_t ADD_BITS = CurLevel::ADD_BITS;
    static constexpr uint8_t TOTAL_BITS = ADD_BITS;

    PageManager();

    PageType* getPage(uint64_t address, bool create, Allocator& allocator);
    Entry* getDirectory();

  private:
    Entry m_entries[1 << ADD_BITS];
};

template <typename Allocator, typename CurLevel, typename... Levels>
PageManager<Allocator, CurLevel, Levels...>::PageManager()
{
  std::memset(this, 0, sizeof(*this));
}

template <typename Allocator, typename CurLevel>
PageManager<Allocator, CurLevel>::PageManager()
{
  std::memset(this, 0, sizeof(*this));
}

template <typename Allocator, typename CurLevel, typename... Levels>
typename PageManager<Allocator, CurLevel, Levels...>::PageType*
  PageManager<Allocator, CurLevel, Levels...>::getPage(
      uint64_t address, bool create, Allocator& allocator)
{
  //debug("descending for ", (uint64_t)address);

  uint64_t addval = reinterpret_cast<uint64_t>(address);
  uint16_t index = (addval >> (TOTAL_BITS - ADD_BITS)) & ((1 << ADD_BITS) - 1);
  NextLayout*& nextLayout = m_nextLayouts[index];

  //debug("index ", (uint64_t)index);
  //debug("add ", (uint64_t)ADD_BITS);
  //debug("total ", (uint64_t)TOTAL_BITS);

  // if not present
  if (!nextLayout)
  {
    if (!create)
      return invalidPtr<PageType>();

    // create it
    void* memory = allocator.allocate(sizeof(NextLayout));
    nextLayout = new (memory) NextLayout();
    //nextLayout = (NextLayout*)memory;
    //nextLayout->init();
    m_entries[index].p = true;
    m_entries[index].base =
      (reinterpret_cast<uint64_t>(nextLayout->getDirectory()) - (0xffffffffc0000000 - 0x800000))
      >> CurLevel::BASE_SHIFT;
  }

  return m_nextLayouts[index]->getPage(address, create, allocator);
}

template <typename Allocator, typename CurLevel>
typename PageManager<Allocator, CurLevel>::PageType*
  PageManager<Allocator, CurLevel>::getPage(
      uint64_t address, bool, Allocator&)
{
  //debug("descending for ", (uint64_t)address);
  //debug("in ", (uint64_t)&prevEntry);

  uint64_t addval = reinterpret_cast<uint64_t>(address);
  uint16_t index = (addval >> (TOTAL_BITS - ADD_BITS)) & ((1 << ADD_BITS) - 1);

  //debug("index ", (uint64_t)index);
  //debug("add ", (uint64_t)ADD_BITS);
  //debug("total ", (uint64_t)TOTAL_BITS);

  return &m_entries[index];
}

template <typename Allocator, typename CurLevel, typename... Levels>
typename PageManager<Allocator, CurLevel, Levels...>::Entry*
  PageManager<Allocator, CurLevel, Levels...>::getDirectory()
{
  return m_entries;
}

template <typename Allocator, typename CurLevel>
typename PageManager<Allocator, CurLevel>::Entry*
  PageManager<Allocator, CurLevel>::getDirectory()
{
  return m_entries;
}

#endif /* GENERIC_PAGING_HPP */
