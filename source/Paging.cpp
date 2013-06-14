#include "Paging.hpp"
#include "KHeap.hpp"
#include "Symbols.hpp"
#include "Memory.hpp"
#include <new>

Paging::CR3 Paging::g_kernel_directory;

StaticMemoryPool g_pool;
Paging::MyPageManager* Paging::g_manager = nullptr;

void Paging::init()
{
  void* poolBase = (void*)(((uint64_t)(((char*)KHeap::kmalloc(0x100000)) + 0x0FFF)) & ~0x0FFFL);
  g_pool.set(poolBase, 0xF0000);

  void* memory = g_pool.allocate(sizeof(MyPageManager));
  g_manager = new (memory) MyPageManager;

  uint8_t* cur = static_cast<uint8_t*>(Symbols::getKernelBootstrapStart());
  uint8_t* end = static_cast<uint8_t*>(Symbols::getKernelBssEnd());
  uint64_t heapShift = (0xffffffffc0000000l - 0x800000);
  uint64_t virtualShift = (0xffffffff80000000l);

  g_kernel_directory.value = 0;
  g_kernel_directory.bitfield.base =
    (reinterpret_cast<uint64_t>(g_manager->getDirectory()) - heapShift) >> 12;

  {
    PageTableEntry* page = g_manager->getPage(0xB8000 >> 12, true, g_pool);
    page->p = true;
    page->rw = true;
    page->base = 0xB8000 >> 12;
  }

  while (cur < end)
  {
    uint8_t* virtualAddress = cur + virtualShift;

    PageTableEntry* page =
      g_manager->getPage(((uint64_t)virtualAddress) >> 12, true, g_pool);
    page->p = true;
    page->rw = true;
    page->base = reinterpret_cast<uint64_t>(cur) >> 12;

    cur += 0x1000;
  }

  cur = (uint8_t*)0x800000;
  end = cur+0x1000000;

  while (cur < end)
  {
    uint8_t* virtualAddress = cur + heapShift;

    PageTableEntry* page =
      g_manager->getPage(((uint64_t)virtualAddress) >> 12, true, g_pool);
    page->p = true;
    page->rw = true;
    page->base = reinterpret_cast<uint64_t>(cur) >> 12;

    cur += 0x1000;
  }

  asm volatile("mov %0, %%cr3":: "r"(g_kernel_directory.value));
}

void Paging::mapPageTo(void* vaddr, uint64_t ipage)
{
  uint64_t ivaddr = reinterpret_cast<uint64_t>(vaddr);

  PageTableEntry* page = g_manager->getPage(ivaddr / 0x1000, true, g_pool);
  // assert(!page->p);
  page->p = true;
  page->rw = true;
  page->base = ipage;
}

void Paging::mapPage(void* vaddr, void** paddr)
{
  uint64_t page = Memory::getFreePage();

  //assert(page != -1);

  mapPageTo(vaddr, page);
  if (paddr)
    *paddr = reinterpret_cast<void*>(page * 0x1000);
}

void Paging::unmapPage(void* vaddr)
{
  uint64_t ivaddr = reinterpret_cast<uint64_t>(vaddr);

  PageTableEntry* page = g_manager->getPage(ivaddr / 0x1000, true, g_pool);
  // assert(page->p);
  page->p = false;
}
