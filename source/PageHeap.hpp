#ifndef PAGE_HEAP_HPP
#define PAGE_HEAP_HPP

#include <cstdint>
#include <vector>
#include <utility>

#include "Types.hpp"

class PageHeap
{
  public:
    using page_index_t = uint32_t;

    static PageHeap& get();

    void init();

    std::pair<void*, physaddr_t> kmalloc();
    void kfree(void* ptr);
    void refillPool();

  private:
    bool m_allocating;
    char* m_heapStart;

    std::vector<bool> m_map;
    std::vector<std::pair<page_index_t, physaddr_t>> m_pool;

    void* pageToPtr(page_index_t index);
    page_index_t ptrToPage(void* ptr);
    std::pair<page_index_t, physaddr_t> allocBlock();
    std::pair<page_index_t, physaddr_t> allocPage(page_index_t index);
};

#endif /* PAGE_HEAP_HPP */
