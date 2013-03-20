#include "Paging.hpp"
#include "KHeap.hpp"

Paging::CR3 Paging::g_kernel_directory;

extern void* Pml4;

void Paging::test(void* root)
{
  CR3 cr3;
  cr3.bitfield.base = ((uint64_t)Pml4) >> 12;
  MyPageManager::getPage(cr3.bitfield, 0, false);
  //MyPageManager::getPage(cr3.bitfield, (void*)0x8000000, false);
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

//void Paging::enable_pae()
//{
//  asm volatile("mov %0, %%cr3":: "r"(g_kernel_directory.value));
//  uint64_t cr4;
//  asm volatile("mov %%cr4, %0": "=r"(cr4));
//  cr4 |= (1 << 5);
//  asm volatile("mov %0, %%cr4":: "r"(cr4));
//}
