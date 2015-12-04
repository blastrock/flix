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
    static constexpr unsigned ATTR_NOEXEC = 0x4;
    static constexpr unsigned ATTR_DEFER  = 0x8;

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

    typedef PageManager<
      PageHeap,
      PageMapLevel4Entry,
      PageDirectoryPointerEntry,
      PageDirectoryEntry,
      PageTableEntry>
        X86_64PageManager;

    PageDirectory();
    PageDirectory(const PageDirectory& pd) = delete;
    PageDirectory(PageDirectory&& pd) noexcept;

    PageDirectory& operator=(const PageDirectory& pd) = delete;
    PageDirectory& operator=(PageDirectory&& pd) noexcept;

    static PageDirectory* initKernelDirectory();
    static PageDirectory* getKernelDirectory();

    static PageDirectory* getCurrent();

    X86_64PageManager& getManager()
    {
      return *m_manager;
    }

    void mapKernel();

    void mapPageTo(void* vaddr, physaddr_t paddr, uint8_t attributes);
    /** Map a virtual address \p vaddr to any free page
     *
     * Return physical address in \p paddr.
     */
    void mapPage(void* vaddr, uint8_t attributes, physaddr_t* paddr = nullptr);

    /// Map a from \p vastart to \p vaend with \p attributes
    void mapRange(void* vastart, void* vaend, uint8_t attributes);

    /// Unmap a page and return the physical address it pointed to
    physaddr_t unmapPage(void* vaddr);

    /** Handles fault due to a deferred allocation
     *
     * \return true if the fault was handled, false otherwise
     */
    bool handleFault(void* vaddr);

    bool isPageMapped(void* vaddr);

    void use();

    static void flushTlb();

  private:
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

    static PageDirectory* g_kernelDirectory;

    CR3 m_directory;
    std::unique_ptr<X86_64PageManager, X86_64PageManager::Deleter> m_manager;

    void createPm();
    void initWithDefaultPaging();

    /// Map \p vaddr to \p paddr with \p attributes
    void _mapPageTo(void* vaddr, physaddr_t paddr, uint8_t attributes);

    /** Map from \p vastart to \p vaend with \p attributes.
     *
     * This will allocate contiguous pages from \p pastart.
     *
     * This function does not assert that paging is ready and does not refill
     * the page pool.
     */
    void mapRangeTo(void* vastart, void* vaend, physaddr_t pastart,
        uint8_t attributes);

    /** Do the real mapping and nothing else
     *
     * Map \p vaddr to \p paddr and apply \p f to the first level page entry.
     *
     * This function only does the mapping. It does not assert that the page
     * directory is ready and it does not refill the page pool after use.
     */
    template <typename F>
    void mapPageToF(void* vaddr, physaddr_t paddr, const F& f);
};

inline PageDirectory::PageDirectory() :
  m_manager(nullptr, [](X86_64PageManager*){})
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
