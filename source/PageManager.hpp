#ifndef PAGE_MANAGER_HPP
#define PAGE_MANAGER_HPP

#include <cstdint>
#include <cstring>
#include <new>
#include "Debug.hpp"

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

    auto getPage(uintptr_t address)
    {
      return getEntry<0>(address);
    }
    template <typename Initializer>
    auto getPage(uintptr_t address, Initializer& initializer)
    {
      return getEntry<0>(address, initializer);
    }
    template <unsigned Level>
    auto getEntry(uintptr_t address)
    {
      return getEntryImpl<LevelCount - Level - 1>(address).first;
    }
    template <unsigned Level, typename Initializer>
    auto getEntry(uintptr_t address, Initializer& initializer)
    {
      return getEntryImpl<LevelCount - Level - 1>(address, initializer).first;
    }

    template <unsigned Level, typename Initializer>
    void mapTo(ThisLayout& other, uintptr_t address, Initializer& initializer)
    {
      static_assert(Level != 0, "Can't map at level 0");

      auto otherPair =
        other.getEntryImpl<LevelCount - Level - 1>(address);

      assert(otherPair.first && "Mapping to unmapped address in 'other'");

      auto thisPair =
        this->getEntryImpl<LevelCount - Level - 1>(address, initializer);
      *thisPair.first = *otherPair.first;
      *thisPair.second = *otherPair.second;
    }

  private:
    Entry m_entries[1 << ADD_BITS];
    NextLayout* m_nextLayouts[1 << ADD_BITS];

    template <unsigned RevLevel>
    auto getEntryImpl(uintptr_t address) ->
      typename std::enable_if<RevLevel != 0,
                 decltype(
                     NextLayout().template getEntryImpl<RevLevel-1>(0))
               >::type;
    template <unsigned RevLevel, typename Initializer>
    auto getEntryImpl(uintptr_t address, Initializer& initializer) ->
      typename std::enable_if<RevLevel != 0,
                 decltype(
                     NextLayout().template getEntryImpl<RevLevel-1>(
                       0, initializer))
               >::type;

    template <unsigned RevLevel>
    auto getEntryImpl(uintptr_t address) ->
      typename std::enable_if<RevLevel == 0, std::pair<Entry*, NextLayout**>>
      ::type;
    template <unsigned RevLevel, typename Initializer>
    auto getEntryImpl(uintptr_t address, Initializer& initializer) ->
      typename std::enable_if<RevLevel == 0, std::pair<Entry*, NextLayout**>>
      ::type;

    inline uintptr_t indexFromAddress(uintptr_t address)
    {
      return (address >> (TOTAL_BITS - ADD_BITS)) & ((1 << ADD_BITS) - 1);
    }

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
    typename std::enable_if<RevLevel == 0, std::pair<Entry*, void**>>::type
      getEntryImpl(uintptr_t address);
    template <unsigned RevLevel, typename Initializer>
    typename std::enable_if<RevLevel == 0, std::pair<Entry*, void**>>::type
      getEntryImpl(uintptr_t address, Initializer& initializer);

    inline uintptr_t indexFromAddress(uintptr_t address)
    {
      return (address >> (TOTAL_BITS - ADD_BITS)) & ((1 << ADD_BITS) - 1);
    }

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
  std::pair<void*, void*> memory = Allocator::get().kmalloc();
  return {new (memory.first) ThisLayout(), memory.second};
}

template <typename Allocator, typename CurLevel>
PageManager<Allocator, CurLevel>::PageManager()
{
  std::memset(this, 0, sizeof(*this));
}

template <typename Allocator, typename CurLevel, typename... NextLevels>
template <unsigned RevLevel>
auto PageManager<Allocator, CurLevel, NextLevels...>::getEntryImpl(
    uintptr_t address) ->
  typename std::enable_if<RevLevel != 0,
             decltype(
                 NextLayout().template getEntryImpl<RevLevel-1>(0))
           >::type
{
  uintptr_t index = indexFromAddress(address);
  NextLayout*& nextLayout = m_nextLayouts[index];

  // if not present
  if (!nextLayout)
  {
    assert(!m_entries[index].p
        && "Incoherence between layouts and page directory");
    return {nullptr, nullptr};
  }

  return m_nextLayouts[index]
    ->template getEntryImpl<RevLevel-1>(address);
}

template <typename Allocator, typename CurLevel, typename... NextLevels>
template <unsigned RevLevel, typename Initializer>
auto PageManager<Allocator, CurLevel, NextLevels...>::getEntryImpl(
    uintptr_t address, Initializer& init) ->
  typename std::enable_if<RevLevel != 0,
             decltype(
                 NextLayout().template getEntryImpl<RevLevel-1>(0, init))
           >::type
{
  uintptr_t index = indexFromAddress(address);
  NextLayout*& nextLayout = m_nextLayouts[index];

  // if not present
  if (!nextLayout)
  {
    assert(!m_entries[index].p
        && "Incoherence between layouts and page directory");
    // create it
    std::pair<void*, void*> memory = Allocator::get().kmalloc();
    nextLayout = new (memory.first) NextLayout();
    m_entries[index].p = true;
    m_entries[index].base =
      reinterpret_cast<uintptr_t>(memory.second) >> CurLevel::BASE_SHIFT;
    init(m_entries[index]);
  }

  return m_nextLayouts[index]
    ->template getEntryImpl<RevLevel-1>(address, init);
}

template <typename Allocator, typename CurLevel, typename... NextLevels>
template <unsigned RevLevel>
auto PageManager<Allocator, CurLevel, NextLevels...>::getEntryImpl(
    uintptr_t address) ->
  typename std::enable_if<RevLevel == 0, std::pair<Entry*, NextLayout**>>::type
{
  uintptr_t index = indexFromAddress(address);

  return {&m_entries[index], &m_nextLayouts[index]};
}

template <typename Allocator, typename CurLevel, typename... NextLevels>
template <unsigned RevLevel, typename Initializer>
auto PageManager<Allocator, CurLevel, NextLevels...>::getEntryImpl(
    uintptr_t address, Initializer&) ->
  typename std::enable_if<RevLevel == 0, std::pair<Entry*, NextLayout**>>::type
{
  uintptr_t index = indexFromAddress(address);

  return {&m_entries[index], &m_nextLayouts[index]};
}

template <typename Allocator, typename CurLevel>
template <unsigned RevLevel>
auto PageManager<Allocator, CurLevel>::getEntryImpl(
    uintptr_t address) ->
  typename std::enable_if<RevLevel == 0, std::pair<Entry*, void**>>::type
{
  uintptr_t index = indexFromAddress(address);

  return {&m_entries[index], nullptr};
}

template <typename Allocator, typename CurLevel>
template <unsigned RevLevel, typename Initializer>
auto PageManager<Allocator, CurLevel>::getEntryImpl(
    uintptr_t address, Initializer&) ->
  typename std::enable_if<RevLevel == 0, std::pair<Entry*, void**>>::type
{
  uintptr_t index = indexFromAddress(address);

  return {&m_entries[index], nullptr};
}

#endif
