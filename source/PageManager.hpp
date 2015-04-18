#ifndef PAGE_MANAGER_HPP
#define PAGE_MANAGER_HPP

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

namespace detail
{

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

}

template <typename Allocator, typename CurLevel, typename... Levels>
class PageManager
{
  public:
    typedef CurLevel Entry;
    typedef PageManager<Allocator, CurLevel, Levels...> ThisLayout;
    typedef PageManager<Allocator, Levels...> NextLayout;
    typedef typename detail::LastType<CurLevel, Levels...>::type PageType;

    static constexpr uint8_t ADD_BITS = CurLevel::ADD_BITS;
    static constexpr uint8_t TOTAL_BITS = ADD_BITS + NextLayout::TOTAL_BITS;

    static std::pair<ThisLayout*, void*> makeNew();

    PageManager();

    PageType* getPage(uintptr_t address, bool create);
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

    PageType* getPage(uintptr_t address, bool create);
    Entry* getDirectory();

  private:
    Entry m_entries[1 << ADD_BITS];
};

template <typename Allocator, typename CurLevel, typename... Levels>
PageManager<Allocator, CurLevel, Levels...>::PageManager()
{
  std::memset(this, 0, sizeof(*this));
}

template <typename Allocator, typename CurLevel, typename... Levels>
std::pair<PageManager<Allocator, CurLevel, Levels...>*, void*> PageManager<Allocator, CurLevel, Levels...>::makeNew()
{
  std::vector<std::pair<void*, void*>> memory =
    Allocator::kmalloc(sizeof(ThisLayout));
  return {new (memory[0].first) ThisLayout(), memory[0].second};
}

template <typename Allocator, typename CurLevel>
PageManager<Allocator, CurLevel>::PageManager()
{
  std::memset(this, 0, sizeof(*this));
}

template <typename Allocator, typename CurLevel, typename... Levels>
typename PageManager<Allocator, CurLevel, Levels...>::PageType*
  PageManager<Allocator, CurLevel, Levels...>::getPage(
      uintptr_t address, bool create)
{
  uintptr_t addval = reinterpret_cast<uintptr_t>(address);
  uintptr_t index = (addval >> (TOTAL_BITS - ADD_BITS)) & ((1 << ADD_BITS) - 1);
  NextLayout*& nextLayout = m_nextLayouts[index];

  // if not present
  if (!nextLayout)
  {
    if (!create)
      return invalidPtr<PageType>();

    // create it
    std::vector<std::pair<void*, void*>> memory =
      Allocator::kmalloc(sizeof(NextLayout));
    nextLayout = new (memory[0].first) NextLayout();
    m_entries[index].p = true;
    m_entries[index].base =
      reinterpret_cast<uintptr_t>(memory[0].second) >> CurLevel::BASE_SHIFT;
  }

  return m_nextLayouts[index]->getPage(address, create);
}

template <typename Allocator, typename CurLevel>
typename PageManager<Allocator, CurLevel>::PageType*
  PageManager<Allocator, CurLevel>::getPage(
      uintptr_t address, bool)
{
  uintptr_t addval = reinterpret_cast<uintptr_t>(address);
  uintptr_t index = (addval >> (TOTAL_BITS - ADD_BITS)) & ((1 << ADD_BITS) - 1);

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

#endif
