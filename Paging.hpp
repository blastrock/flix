#ifndef PAGING_HPP
#define PAGING_HPP

#include "inttypes.hpp"

class Paging
{
  public:

    /**
      Sets up the environment, page directories etc and
      enables paging.
     **/
    static void initialise_paging();

  private:
    struct Page
    {
      unsigned present    : 1;   // Page present in memory
      unsigned rw         : 1;   // Read-only if clear, readwrite if set
      unsigned user       : 1;   // Supervisor level only if clear
      unsigned accessed   : 1;   // Has the page been accessed since last refresh?
      unsigned dirty      : 1;   // Has the page been written to since last refresh?
      unsigned unused     : 7;   // Amalgamation of unused and reserved bits
      unsigned frame      : 20;  // Frame address (shifted right 12 bits)
    };

    struct PageTable
    {
      Page pages[1024];
    };

    struct PageDirectory
    {
      PageTable* tables[1024];
      u32 tablesDir[1024];
    };

    static PageDirectory* g_kernel_directory;

    /**
      Causes the specified page directory to be loaded into the
      CR3 register.
     **/
    static void switch_page_directory(PageDirectory* pd);

    /**
      Retrieves a pointer to the page required.
      If make == 1, if the page-table in which this page should
      reside isn't created, create it!
     **/
    static Page* get_page(u32 address, int make, PageDirectory *dir);

    /**
      Handler for page faults.
     **/
    //void page_fault(registers_t regs
};

#endif /* PAGING_HPP */
