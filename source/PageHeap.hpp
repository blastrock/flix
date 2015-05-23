#ifndef PAGE_HEAP_HPP
#define PAGE_HEAP_HPP

#include <cstdint>
#include <vector>
#include <utility>

class PageHeap
{
  public:
    static void init();

    static std::pair<void*, void*> kmalloc();
    static void kfree(void* ptr);

  private:
    static bool m_allocating;
    static char* m_heapStart;

    static std::vector<bool> m_map;
    static std::vector<std::pair<uint64_t, void*>> m_pool;

    static void* pageToPtr(uint64_t index);
    static std::pair<uint64_t, void*> allocBlock();
    static std::pair<uint64_t, void*> allocPage(uint64_t index);
    static void refillPool();
};

#endif /* PAGE_HEAP_HPP */
