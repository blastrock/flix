#include "Paging.hpp"
#include "KHeap.hpp"
#include "Symbols.hpp"
#include "Memory.hpp"
#include <new>

Paging::CR3 Paging::g_kernel_directory;

Paging::MyPageManager* Paging::g_manager = nullptr;

void Paging::init()
{
  fDeg() << "creating pml4";
  std::pair<MyPageManager*, void*> pm = MyPageManager::makeNew();
  g_manager = pm.first;

  g_kernel_directory.value = 0;
  g_kernel_directory.bitfield.base =
    reinterpret_cast<uint64_t>(pm.second) >> 12;

  fDeg() << "mapping vga";
  {
    PageTableEntry* page = g_manager->getPage(0xB8000 >> 12, true);
    page->p = true;
    page->rw = true;
    page->base = 0xB8000 >> 12;
  }

  // mapping .text
  fDeg() << "mapping .text";
  uint64_t vcur = reinterpret_cast<uint64_t>(Symbols::getKernelVTextStart());
  uint64_t cur = reinterpret_cast<uint64_t>(Symbols::getKernelTextStart());
  uint64_t end = reinterpret_cast<uint64_t>(Symbols::getKernelBssEnd());
  while (cur < end)
  {
    PageTableEntry* page = g_manager->getPage(vcur >> 12, true);
    page->p = true;
    page->rw = true;
    page->base = cur >> 12;

    cur += 0x1000;
    vcur += 0x1000;
  }

  // mapping stack
  fDeg() << "mapping stack";
  cur = 0x800000 + 0x200000 - 0x4000;
  end = cur + 0x4000;
  vcur = 0xffffffff90000000 - 0x4000;
  while (cur < end)
  {
    PageTableEntry* page = g_manager->getPage(vcur >> 12, true);
    page->p = true;
    page->rw = true;
    page->base = cur >> 12;

    cur += 0x1000;
    vcur += 0x1000;
  }

  // mapping page heap
  fDeg() << "mapping page heap";
  vcur = 0xffffffffa0000000;
  cur = 0xa00000;
  end = cur + 0x200000;
  while (cur < end)
  {
    PageTableEntry* page = g_manager->getPage(vcur >> 12, true);
    page->p = true;
    page->rw = true;
    page->base = cur >> 12;

    cur += 0x1000;
    vcur += 0x1000;
  }

  // mapping heap
  fDeg() << "mapping heap";
  vcur = 0xffffffffb0000000;
  cur = 0xc00000;
  end = cur + 0x200000;
  while (cur < end)
  {
    PageTableEntry* page = g_manager->getPage(vcur >> 12, true);
    page->p = true;
    page->rw = true;
    page->base = cur >> 12;

    cur += 0x1000;
    vcur += 0x1000;
  }

  fDeg() << "changing pagetable to " << std::hex << g_kernel_directory.value;
  asm volatile("mov %0, %%cr3":: "r"(g_kernel_directory.value));
}

void Paging::mapPageTo(void* vaddr, uint64_t ipage)
{
  uint64_t ivaddr = reinterpret_cast<uint64_t>(vaddr);

  PageTableEntry* page = g_manager->getPage(ivaddr / 0x1000, true);
  assert(!page->p);
  page->p = true;
  page->rw = true;
  page->base = ipage;
}

void Paging::mapPage(void* vaddr, void** paddr)
{
  uint64_t page = Memory::getFreePage();

  assert(page != (uint64_t)-1);

  mapPageTo(vaddr, page);
  if (paddr)
    *paddr = reinterpret_cast<void*>(page * 0x1000);
}

void Paging::unmapPage(void* vaddr)
{
  uint64_t ivaddr = reinterpret_cast<uint64_t>(vaddr);

  PageTableEntry* page = g_manager->getPage(ivaddr / 0x1000, true);
  assert(page->p);
  page->p = false;
}
