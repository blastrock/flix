#include "Paging.hpp"
#include "KHeap.hpp"
#include <new>

Paging::CR3 Paging::g_kernel_directory;

extern "C" uint8_t Pml4;
extern "C" uint8_t _kernelStart;
extern "C" uint8_t _kernelBssEnd;
extern "C" uint8_t VIRTUAL_BASE;

void Paging::init()
{
  void* poolBase = (void*)(((uint64_t)(((char*)KHeap::kmalloc_a(0x100000)) + 0x0FFF)) & ~0x0FFFL);
  StaticMemoryPool pool(poolBase, 0x100000);

  void* memory = pool.allocate(sizeof(MyPageManager));
  MyPageManager* manager = new (memory) MyPageManager;

  uint8_t* cur = &_kernelStart;
  uint8_t* end = &_kernelBssEnd;
  uint64_t heapShift = (0xffffffffc0000000l - 0x800000);
  uint64_t virtualShift = (0xffffffff80000000l);

  g_kernel_directory.value = 0;
  g_kernel_directory.bitfield.base =
    (reinterpret_cast<uint64_t>(manager->getDirectory()) - heapShift) >> 12;

  {
    debug("mapping ", (uint64_t)cur);

    PageTableEntry* page = manager->getPage(0xB8000 >> 12, true, pool);
    page->p = true;
    page->rw = true;
    page->base = 0xB8000 >> 12;
  }

  while (cur < end)
  {
    debug("mapping ", (uint64_t)cur);

    uint8_t* virtualAddress = cur + virtualShift;

    PageTableEntry* page =
      manager->getPage(((uint64_t)virtualAddress) >> 12, true, pool);
    page->p = true;
    page->rw = true;
    page->base = reinterpret_cast<uint64_t>(cur) >> 12;

    cur += 0x1000;
  }

  cur = (uint8_t*)0x800000;
  end = cur+0x200000;

  while (cur < end)
  {
    debug("mapping ", (uint64_t)cur);

    uint8_t* virtualAddress = cur + heapShift;

    PageTableEntry* page =
      manager->getPage(((uint64_t)virtualAddress) >> 12, true, pool);
    page->p = true;
    page->rw = true;
    page->base = reinterpret_cast<uint64_t>(cur) >> 12;

    cur += 0x1000;
  }

  debug("shift : ", virtualShift);
  debug("heap : ", heapShift);
  debug("lol :", (uint64_t)manager->getDirectory());

  debug("stack :", (uint64_t)&cur);

  asm volatile("mov %0, %%cr3":: "r"(g_kernel_directory.value));
}
