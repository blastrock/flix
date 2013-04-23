#include "Paging.hpp"
#include "KHeap.hpp"

Paging::CR3 Paging::g_kernel_directory;

extern "C" uint8_t Pml4;
extern "C" uint8_t _kernelStart;
extern "C" uint8_t _kernelBssEnd;
extern "C" uint8_t VIRTUAL_BASE;

void Paging::test(void* root)
{
  CR3 cr3;
  cr3.bitfield.base = ((uint64_t)&Pml4) >> 12;
  //MyPageManager::getPage(cr3.bitfield, 0, false);
  //MyPageManager::getPage(cr3.bitfield, (void*)0x8000000, false);
}

void Paging::init()
{
  void* poolBase = (void*)(((uint64_t)(((char*)KHeap::kmalloc_a(0x100000)) + 0x0FFF)) & ~0x0FFFL);
  StaticMemoryPool pool(poolBase, 0x100000);

  void* memory = pool.allocate(sizeof(MyPageManager));
  //MyPageManager* manager = new (memory) MyPageManager;
  MyPageManager* manager = static_cast<MyPageManager*>(memory);
  manager->init();

  uint8_t* cur = &_kernelStart;
  uint8_t* end = &_kernelBssEnd;
  uint64_t heapShift = (0xffffffffc0000000l - 0x800000);
  uint64_t virtualShift = (0xffffffff80000000l);

  g_kernel_directory.value = 0;
  g_kernel_directory.bitfield.base =
    (reinterpret_cast<uint64_t>(manager->getDirectory()) - heapShift) >> 12;

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
  debug("shift : ", virtualShift);
  debug("heap : ", heapShift);
  debug("lol :", (uint64_t)manager->getDirectory());

  debug("stack :", (uint64_t)&cur);

  enablePaging();
}

//void Paging::initializePaging(void* root)
//{
//  CR3* cr3 = reinterpret_cast<CR3*>(root);
//
//  PageMapLevel4Entry* pml4 = reinterpret_cast<PageMapLevel4Entry*>(
//      cr3->base << 12);
//  PageMapLevel4Entry* pml4End = pml4 + 512;
//
//  for (; pml4 < pml4End; ++pml4)
//    updateFree(pml4);
//}
//
//template <typename T>
//void Paging::updateFree(T* entry)
//{
//  bool free = true;
//
//  for (T* e = reinterpret_cast<T*>(
//        entry->base << 12), end = e + 512;
//      e < end; ++e)
//    updateFree();
//}

void Paging::enablePaging()
{
  debug("cr3: ", g_kernel_directory.value);
  asm volatile("mov %0, %%cr3":: "r"(g_kernel_directory.value));
  //uint64_t cr4;
  //asm volatile("mov %%cr4, %0": "=r"(cr4));
  //cr4 |= (1 << 5);
  //asm volatile("mov %0, %%cr4":: "r"(cr4));
}
