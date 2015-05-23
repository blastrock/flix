#ifndef PAGE_HEAP_HPP
#define PAGE_HEAP_HPP

#include <cstdint>
#include <vector>
#include <utility>

class PageHeap
{
  public:
    static PageHeap& get();

    void init();

    std::pair<void*, void*> kmalloc();
    void kfree(void* ptr);

  private:
    bool m_allocating;
    char* m_heapStart;

    std::vector<bool> m_map;
    std::vector<std::pair<uint64_t, void*>> m_pool;

    void* pageToPtr(uint64_t index);
    std::pair<uint64_t, void*> allocBlock();
    std::pair<uint64_t, void*> allocPage(uint64_t index);
    void refillPool();
};

#endif /* PAGE_HEAP_HPP */
