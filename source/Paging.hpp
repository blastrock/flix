#ifndef PAGING_HPP
#define PAGING_HPP

#include <cstdint>
#include "PageManager.hpp"

class Paging
{
  public:
    static constexpr uint8_t ADDRESSING_BITS = 52;

    static void initializePaging(void* root);
    static void test(void* root);

  private:
    struct PageTableEntry
    {
      static constexpr uint8_t ADD_BITS = 9;
      static constexpr uint8_t BASE_SHIFT = 12;

      unsigned long long p : 1; ///< Present
      unsigned long long rw : 1; ///< Read/Write
      unsigned long long us : 1; ///< User/Supervisor
      unsigned long long pwt : 1; ///< Page-Level Writethrough
      unsigned long long pcd : 1; ///< Page-Level Cache Disable
      unsigned long long a : 1; ///< Accessed
      unsigned long long d : 1; ///< Dirty
      unsigned long long pat : 1; ///< Page-Attribute Table
      unsigned long long g : 1; ///< Global Page
      unsigned long long hasFree : 1; ///< OS-defined: has free pages
      unsigned long long avl : 2; ///< Available to Software
      unsigned long long base : 40; ///< Page Base Address
      unsigned long long avl2 : 11; ///< Available
      unsigned long long nx : 1; ///< No Execute
    };

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

        unsigned long long res : 3; ///< Reserved
        unsigned long long pwt : 1; ///< Page-Level Writethrough
        unsigned long long pcd : 1; ///< Page-Level Cache Disable
        unsigned long long res2 : 7; ///< Reserved
        unsigned long long base : 40; ///< Page Map Level 4 base address
        unsigned long long mbz : 12; ///< Reserved, must be zero
      } bitfield;
      uint64_t value;
    };

    typedef PageManager<
      CR3::CR3Bitfield,
      PageMapLevel4Entry,
      PageDirectoryPointerEntry,
      PageDirectoryEntry,
      PageTableEntry>
        MyPageManager;

    static CR3 g_kernel_directory;
};

#endif /* PAGING_HPP */