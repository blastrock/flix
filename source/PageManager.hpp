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

template <unsigned Type, typename T0, typename... T>
struct GetNthType
{
  typedef typename GetNthType<Type - 1, T...>::type type;
};

template <typename T0, typename... T>
struct GetNthType<0, T0, T...>
{
  typedef T0 type;
};

}

template <typename Allocator, typename CurLevel, typename... NextLevels>
class PageManager
{
  public:
    typedef std::tuple<CurLevel, NextLevels...> Levels;
    typedef typename std::tuple_element<
      std::tuple_size<Levels>::value - 1, Levels>::type PageType;
    typedef CurLevel Entry;
    typedef PageManager<Allocator, CurLevel, NextLevels...> ThisLayout;
    typedef PageManager<Allocator, NextLevels...> NextLayout;

    static constexpr uint8_t ADD_BITS = CurLevel::ADD_BITS;
    static constexpr uint8_t TOTAL_BITS = ADD_BITS + NextLayout::TOTAL_BITS;

    static constexpr uint8_t LevelCount = std::tuple_size<Levels>::value;

    static std::pair<ThisLayout*, void*> makeNew();

    PageManager();

    auto getPage(uintptr_t address, bool create)
    {
      return getEntry<0>(address, create);
    }
    template <unsigned Level>
    auto getEntry(uintptr_t address, bool create)
    {
      return getEntryImpl<LevelCount - Level - 1>(address, create);
    }

  private:
    Entry m_entries[1 << ADD_BITS];
    NextLayout* m_nextLayouts[1 << ADD_BITS];

    template <unsigned RevLevel>
    auto getEntryImpl(uintptr_t address, bool create) ->
      typename std::enable_if<RevLevel != 0,
                 decltype(
                     std::declval<NextLayout>().template getEntryImpl<RevLevel-1>(
                       std::declval<uintptr_t>(), std::declval<bool>()))
               >::type;

    template <unsigned RevLevel>
    auto getEntryImpl(uintptr_t address, bool create) ->
      typename std::enable_if<RevLevel == 0, Entry*>::type;

    template <typename A, typename Clv, typename... Lv>
    friend class PageManager;
};

template <typename Allocator, typename CurLevel>
class PageManager<Allocator, CurLevel>
{
  public:
    typedef CurLevel Entry;
    typedef CurLevel PageType;

    static constexpr uint8_t ADD_BITS = CurLevel::ADD_BITS;
    static constexpr uint8_t TOTAL_BITS = ADD_BITS + CurLevel::BASE_SHIFT;

    static constexpr uint8_t Levels = 1;

    PageManager();

  private:
    Entry m_entries[1 << ADD_BITS];
    template <unsigned RevLevel>
    typename std::enable_if<RevLevel == 0, Entry*>::type
      getEntryImpl(uintptr_t address, bool create);

    template <typename A, typename Clv, typename... Lv>
    friend class PageManager;
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

template <typename Allocator, typename CurLevel, typename... NextLevels>
template <unsigned RevLevel>
auto PageManager<Allocator, CurLevel, NextLevels...>::getEntryImpl(
    uintptr_t address, bool create) ->
  typename std::enable_if<RevLevel != 0,
             decltype(
                 std::declval<NextLayout>().template getEntryImpl<RevLevel-1>(
                   std::declval<uintptr_t>(), std::declval<bool>()))
           >::type
{
  uintptr_t index = (address >> (TOTAL_BITS - ADD_BITS)) & ((1 << ADD_BITS) - 1);
  NextLayout*& nextLayout = m_nextLayouts[index];

  // if not present
  if (!nextLayout)
  {
    if (!create)
      return invalidPtr<typename std::remove_pointer<decltype(
                 std::declval<NextLayout>().template getEntryImpl<RevLevel-1>(
                   std::declval<uintptr_t>(), std::declval<bool>()))>::type>();

    // create it
    std::vector<std::pair<void*, void*>> memory =
      Allocator::kmalloc(sizeof(NextLayout));
    nextLayout = new (memory[0].first) NextLayout();
    m_entries[index].p = true;
    m_entries[index].base =
      reinterpret_cast<uintptr_t>(memory[0].second) >> CurLevel::BASE_SHIFT;
  }

  return m_nextLayouts[index]->template getEntryImpl<RevLevel-1>(address, create);
}

template <typename Allocator, typename CurLevel, typename... NextLevels>
template <unsigned RevLevel>
auto PageManager<Allocator, CurLevel, NextLevels...>::getEntryImpl(
    uintptr_t address, bool) ->
  typename std::enable_if<RevLevel == 0, Entry*>::type
{
  uintptr_t index =
    (address >> (TOTAL_BITS - ADD_BITS)) & ((1 << ADD_BITS) - 1);

  return &m_entries[index];
}

template <typename Allocator, typename CurLevel>
template <unsigned RevLevel>
auto PageManager<Allocator, CurLevel>::getEntryImpl(
    uintptr_t address, bool) ->
  typename std::enable_if<RevLevel == 0, Entry*>::type
{
  uintptr_t index =
    (address >> (TOTAL_BITS - ADD_BITS)) & ((1 << ADD_BITS) - 1);

  return &m_entries[index];
}

#endif
