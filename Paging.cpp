#include "Paging.hpp"
#include "KHeap.hpp"
#include "Debug.hpp"

Paging::PageDirectory* Paging::g_kernel_directory = nullptr;

void Paging::initialise_paging()
{
  // Let's make a page directory.
  g_kernel_directory = (PageDirectory*)KHeap::kmalloc_a(sizeof(PageDirectory));
  memset(g_kernel_directory, 0, sizeof(PageDirectory));

  // We need to identity map (phys addr = virt addr) from
  // 0x0 to the end of used memory, so we can access this
  // transparently, as if paging wasn't enabled.
  // NOTE that we use a while loop here deliberately.
  // inside the loop body we actually change placement_address
  // by calling kmalloc(). A while loop causes this to be
  // computed on-the-fly rather than once at the start.
  char* i = 0;
  while (((u32)i) < 0x4000000)
  {
    // Kernel code is readable but not writeable from userspace.
    Page* page = get_page((u32)i, 1, g_kernel_directory);
    page->present = 1; // Mark it as present.
    page->rw = 1; // Should the page be writeable?
    page->user = 0; // Should the page be user-mode?
    page->frame = ((u32)i)/0x1000;
    i += 0x1000;
  }
  debug("last add", (u32)i);
  debug("stack :", (u32)&i);
  // Before we enable paging, we must register our page fault handler.
  //register_interrupt_handler(14, page_fault);

  // Now, enable paging!
  switch_page_directory(g_kernel_directory);
}

void Paging::switch_page_directory(PageDirectory *dir)
{
  asm volatile("mov %0, %%cr3":: "r"(&dir->tablesDir));
  u32 cr0;
  asm volatile("mov %%cr0, %0": "=r"(cr0));
  cr0 |= 0x80000000; // Enable paging!
  asm volatile("mov %0, %%cr0":: "r"(cr0));
}

Paging::Page* Paging::get_page(u32 address, int make, PageDirectory *dir)
{
  // Turn the address into an index.
  address /= 0x1000;
  // Find the page table containing this address.
  u32 table_idx = address / 1024;
  if (dir->tables[table_idx]) // If this table is already assigned
  {
    return &dir->tables[table_idx]->pages[address%1024];
  }
  else if(make)
  {
    u32 tmp;
    dir->tables[table_idx] = (PageTable*)KHeap::kmalloc_a(sizeof(PageTable), (void**)&tmp);
    memset(dir->tables[table_idx], 0, 0x1000);
    dir->tablesDir[table_idx] = tmp | 0x7; // PRESENT, RW, US.
    return &dir->tables[table_idx]->pages[address%1024];
  }
  else
  {
    return 0;
  }
}
