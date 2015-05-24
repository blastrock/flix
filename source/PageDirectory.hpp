#ifndef PAGE_DIRECTORY_HPP
#define PAGE_DIRECTORY_HPP

#include <cstdint>
#include "PageHeap.hpp"
#include "PageManager.hpp"

class PageDirectory
{
  public:
    static constexpr unsigned BASE_SHIFT = 12;

    static constexpr unsigned ATTR_RW     = 0x1;
    static constexpr unsigned ATTR_PUBLIC = 0x2;

    PageDirectory();
    PageDirectory(const PageDirectory& pd) = delete;
    PageDirectory(PageDirectory&& pd) noexcept;

    PageDirectory& operator=(const PageDirectory& pd) = delete;
    PageDirectory& operator=(PageDirectory&& pd) noexcept;

    static PageDirectory* initKernelDirectory();
    static PageDirectory* getKernelDirectory();

    static PageDirectory* getCurrent();

    void mapKernel();

    void mapPageTo(void* vaddr, page_t page, uint8_t attributes);
    void mapPage(void* vaddr, uint8_t attributes, physaddr_t* paddr = nullptr);
    physaddr_t unmapPage(void* vaddr);

    bool isPageMapped(void* vaddr);

    void use();

    static void flushTlb();

  private:
    static PageDirectory* g_kernelDirectory;

    struct PageTableEntry
    {
      static constexpr uint8_t ADD_BITS = 9;
      static constexpr uint8_t BASE_SHIFT = ::PageDirectory::BASE_SHIFT;

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
        static constexpr uint8_t BASE_SHIFT = ::PageDirectory::BASE_SHIFT;
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
        X86_64PageManager;

    CR3 m_directory;
    std::unique_ptr<X86_64PageManager, X86_64PageManager::Deleter> m_manager;

    void createPm();
    void initWithDefaultPaging();
    void mapRangeTo(void* vastart, void* vaend, physaddr_t pastart,
        uint8_t attributes);
    void mapAddrTo(void* ivaddr, physaddr_t ipaddr, uint8_t attributes);
    void _mapPageTo(void* vaddr, page_t page, uint8_t attributes);
    template <typename F>
    void mapPageToF(void* vaddr, page_t ipage, const F& f);
};

inline PageDirectory::PageDirectory() :
  m_manager(nullptr, [](auto){})
{
  m_directory.value = 0;
}

inline PageDirectory::PageDirectory(PageDirectory&& pd) :
  m_directory(pd.m_directory),
  m_manager(std::move(pd.m_manager))
{
  pd.m_directory.value = 0;
  pd.m_manager = nullptr;
}

inline PageDirectory& PageDirectory::operator=(PageDirectory&& pd)
{
  m_directory = pd.m_directory;
  m_manager = std::move(pd.m_manager);

  pd.m_directory.value = 0;
  pd.m_manager = nullptr;

  return *this;
}

inline PageDirectory* PageDirectory::getKernelDirectory()
{
  return g_kernelDirectory;
}

#endif
