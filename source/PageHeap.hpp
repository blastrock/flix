#ifndef PAGE_HEAP_HPP
#define PAGE_HEAP_HPP

#include <cstdint>
#include <vector>
#include <utility>

class PageHeap
{
  public:
    static void init();

    static std::pair<void*, void*> kmalloc(uint64_t size);
    static void kfree(void* ptr);

  private:
    static bool m_allocating;
    static uint8_t* m_heapStart;

    static std::vector<bool> m_map;
    static std::vector<std::pair<uint64_t, void*>> m_pool;

    static void* pageToPtr(uint64_t index);
    static std::pair<uint64_t, void*> allocPages(uint64_t nbPages);
    static std::pair<uint64_t, void*> allocPage(uint64_t index);
    static void refillPool();
};

#endif /* PAGE_HEAP_HPP */
