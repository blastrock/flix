#include "PageDirectory.hpp"
#include "KHeap.hpp"
#include "Symbols.hpp"
#include "Memory.hpp"

PageDirectory* PageDirectory::g_kernelDirectory = nullptr;

PageDirectory* PageDirectory::initKernelDirectory()
{
  if (!g_kernelDirectory)
  {
    g_kernelDirectory = new PageDirectory();
    g_kernelDirectory->initWithDefaultPaging();
  }
  return g_kernelDirectory;
}

static const auto PUBLIC_RW = [](auto& e) { e.us = true; e.rw = true; };
static const auto PUBLIC_RO = [](auto& e) { e.us = true; e.rw = false; };
static const auto PRIVATE_RW = [](auto& e) { e.us = false; e.rw = true; };
static const auto PRIVATE_RO = [](auto& e) { e.us = false; e.rw = false; };

// This function reuses the kernel's page directory for the upper adresses
void PageDirectory::mapKernel()
{
  // TODO this function should not initialize, or its name should be changed
  // TODO this code is copy pasted from below
  std::pair<X86_64PageManager*, void*> pm = X86_64PageManager::makeNew();
  m_manager = pm.first;

  m_directory.value = 0;
  m_directory.bitfield.base = reinterpret_cast<uintptr_t>(pm.second) >> 12;

  m_manager->mapTo<2>(*getKernelDirectory()->m_manager, 0xffffffffc0000000,
      PUBLIC_RW);
}

#ifndef NDEBUG
static bool g_pagingReady = false;
#endif

void PageDirectory::initWithDefaultPaging()
{
  std::pair<X86_64PageManager*, void*> pm = X86_64PageManager::makeNew();
  m_manager = pm.first;

  m_directory.value = 0;
  m_directory.bitfield.base =
    reinterpret_cast<uintptr_t>(pm.second) >> BASE_SHIFT;

  // map VGA
  mapAddrTo(reinterpret_cast<void*>(0xB8000), 0xB8000);
  Memory::setPageUsed(0xB8000 / 0x1000);

  // mapping .text
  mapRangeTo(
      Symbols::getKernelVTextStart(),
      Symbols::getKernelVBssEnd(),
      Symbols::getKernelTextStart());
  Memory::setRangeUsed(
      Symbols::getKernelTextStart() / 0x1000,
      (Symbols::getKernelTextStart() +
       Symbols::getKernelVBssEnd() - Symbols::getKernelVTextStart()) / 0x1000);

  // mapping stack
  mapRangeTo(
      Symbols::getStackBase() - 0x4000,
      Symbols::getStackBase(),
      0x800000 + 0x200000 - 0x4000);
  Memory::setRangeUsed(
      (0x800000 + 0x200000 - 0x4000) / 0x1000,
      0x4000 / 0x1000);

  // mapping page heap
  mapRangeTo(
      Symbols::getPageHeapBase(),
      Symbols::getPageHeapBase() + 32*0x1000,
      0xa00000);
  Memory::setRangeUsed(
      0xa00000 / 0x1000,
      (0xa00000 + 32*0x1000) / 0x1000);

  // mapping heap
  mapRangeTo(
      Symbols::getHeapBase(),
      Symbols::getHeapBase() + 0x200000,
      0xc00000);
  Memory::setRangeUsed(
      0xc00000 / 0x1000,
      (0xc00000 + 0x200000) / 0x1000);
}

static PageDirectory* g_currentPageDirectory = 0;

void PageDirectory::use()
{
#ifndef NDEBUG
  g_pagingReady = true;
#endif

  Degf("Changing pagetable to %x", m_directory.value);
  asm volatile("mov %0, %%cr3":: "r"(m_directory.value));
  g_currentPageDirectory = this;
}

PageDirectory* PageDirectory::getCurrent()
{
  return g_currentPageDirectory;
}

void PageDirectory::mapPageTo(uintptr_t ivaddr, uintptr_t ipage)
{
  Degf("Mapping %x to %x", ivaddr, ipage << BASE_SHIFT);

  PageTableEntry* page = m_manager->getPage(ivaddr, PUBLIC_RW);
  assert(!page->p && "Page already mapped");
  // TODO move this, only system pages may be mapped like this, which is bad
  page->p = true;
  PUBLIC_RW(*page);
  page->base = ipage;
}

void PageDirectory::mapAddrTo(void* ivaddr, uintptr_t ipaddr)
{
  mapPageTo(reinterpret_cast<uintptr_t>(ivaddr), ipaddr >> BASE_SHIFT);
}

void PageDirectory::mapRangeTo(void* vastart, void* vaend, uintptr_t pastart)
{
  uintptr_t ivastart = reinterpret_cast<uintptr_t>(vastart);
  uintptr_t ivaend = reinterpret_cast<uintptr_t>(vaend);

  while (ivastart < ivaend)
  {
    mapAddrTo(reinterpret_cast<void*>(ivastart), pastart);

    ivastart += 0x1000;
    pastart += 0x1000;
  }
}

void PageDirectory::mapPageTo(void* vaddr, uintptr_t ipage)
{
  assert(g_pagingReady);

  mapPageTo(reinterpret_cast<uintptr_t>(vaddr), ipage);
}

void PageDirectory::mapPage(void* vaddr, void** paddr)
{
  assert(g_pagingReady);

  uintptr_t page = Memory::getFreePage();

  assert(page != static_cast<uintptr_t>(-1));

  mapPageTo(vaddr, page);
  if (paddr)
    *paddr = reinterpret_cast<void*>(page * 0x1000);
}

void PageDirectory::unmapPage(void* vaddr)
{
  assert(g_pagingReady);

  uintptr_t ivaddr = reinterpret_cast<uintptr_t>(vaddr);

  PageTableEntry* page = m_manager->getPage(ivaddr);
  assert(page && page->p && "Unmapping page that was not mapped");
  page->p = false;

  Memory::setPageFree(ivaddr / 0x1000);
}

bool PageDirectory::isPageMapped(void* vaddr)
{
  assert(g_pagingReady);

  uintptr_t ivaddr = reinterpret_cast<uintptr_t>(vaddr);
  PageTableEntry* page = m_manager->getPage(ivaddr);
  return page && page->p;
}

// TODO make something to invalidate a single page (invlpg)
void PageDirectory::flushTlb()
{
  asm volatile(
      "mov %%cr3, %%rax\n"
      "mov %%rax, %%cr3\n"
      :::"rax");
}
