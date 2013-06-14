#ifndef PAGING_HPP
#define PAGING_HPP

#include <cstdint>
#include "PageHeap.hpp"
#include "PageManager.hpp"

class Paging
{
  public:
    static constexpr uint8_t ADDRESSING_BITS = 52;

    static void init();

    static void mapPageTo(void* vaddr, uint64_t page);
    static void mapPage(void* vaddr, void** paddr = nullptr);
    static void unmapPage(void* vaddr);

  private:
    struct PageTableEntry
    {
      static constexpr uint8_t ADD_BITS = 9;
      static constexpr uint8_t BASE_SHIFT = 12;

      unsigned long long p    : 1; ///< Present
      unsigned long long rw   : 1; ///< Read/Write
      unsigned long long us   : 1; ///< User/Supervisor
      unsigned long long pwt  : 1; ///< Page-Level Writethrough
      unsigned long long pcd  : 1; ///< Page-Level Cache Disable
      unsigned long long a    : 1; ///< Accessed
      unsigned long long d    : 1; ///< Dirty
      unsigned long long pat  : 1; ///< Page-Attribute Table
      unsigned long long g    : 1; ///< Global Page
      unsigned long long avl  : 3; ///< Available to Software
      unsigned long long base : 40; ///< Page Base Address
      unsigned long long avl2 : 11; ///< Available
      unsigned long long nx   : 1; ///< No Execute
    };

    static_assert(sizeof(PageTableEntry) == 8, "sizeof(PageTableEntry) != 8");

    typedef PageTableEntry PageDirectoryEntry;
    typedef PageTableEntry PageDirectoryPointerEntry;
    typedef PageTableEntry PageMapLevel4Entry;

    union CR3
    {
      struct CR3Bitfield
      {
        static constexpr uint8_t ADD_BITS = 0;
        static constexpr uint8_t BASE_SHIFT = 12;
        static constexpr uint8_t p = 1;

        unsigned long long res  : 3; ///< Reserved
        unsigned long long pwt  : 1; ///< Page-Level Writethrough
        unsigned long long pcd  : 1; ///< Page-Level Cache Disable
        unsigned long long res2 : 7; ///< Reserved
        unsigned long long base : 40; ///< Page Map Level 4 base address
        unsigned long long mbz  : 12; ///< Reserved, must be zero
      } bitfield;
      uint64_t value;
    };

    static_assert(sizeof(CR3) == 8, "sizeof(CR3) != 8");

    typedef PageManager<
      PageHeap,
      PageMapLevel4Entry,
      PageDirectoryPointerEntry,
      PageDirectoryEntry,
      PageTableEntry>
        MyPageManager;

    static CR3 g_kernel_directory;
    static MyPageManager* g_manager;
};

#endif /* PAGING_HPP */
