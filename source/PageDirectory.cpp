#include "PageDirectory.hpp"
#include "KHeap.hpp"
#include "Symbols.hpp"
#include "Memory.hpp"
#include "Debug.hpp"

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

template <unsigned Attr>
static const auto AttributeSetter = [](auto& e) {
  if (Attr & PageDirectory::ATTR_RW)
    e.rw = true;
  if (Attr & PageDirectory::ATTR_PUBLIC)
    e.us = true;
};

void PageDirectory::createPm()
{
  auto pm = X86_64PageManager::makeNew();
  m_manager = std::move(pm.ptr);

  m_directory.value = 0;
  m_directory.bitfield.base = pm.physAddr >> BASE_SHIFT;
}

// This function reuses the kernel's page directory for the upper adresses
void PageDirectory::mapKernel()
{
  // TODO this function should not initialize, or its name should be changed
  createPm();

  m_manager->mapTo<2>(*getKernelDirectory()->m_manager,
      Symbols::getKernelVBase(),
      AttributeSetter<ATTR_RW>);

  m_manager->getEntry<3>(reinterpret_cast<void*>(0xffffffff00000000))->us = 1;
}

#ifndef NDEBUG
static bool g_pagingReady = false;
#endif

void PageDirectory::initWithDefaultPaging()
{
  createPm();

  // map VGA
  mapAddrTo(Symbols::getKernelVTextStart() + 0x01000000, 0xB8000, ATTR_RW);
  Memory::setPageUsed(0xB8000 / 0x1000);

  // mapping .text and .rodata
  mapRangeTo(
      Symbols::getKernelVTextStart(),
      Symbols::getKernelVRodataEnd(),
      Symbols::getKernelTextStart(),
      0);
  Memory::setRangeUsed(
      Symbols::getKernelTextStart() / 0x1000,
      (Symbols::getKernelTextStart() +
       Symbols::getKernelVRodataEnd() - Symbols::getKernelVTextStart()) / 0x1000);

  // mapping .data and .bss
  mapRangeTo(
      Symbols::getKernelVDataStart(),
      Symbols::getKernelVBssEnd(),
      Symbols::getKernelDataStart(),
      ATTR_RW);
  Memory::setRangeUsed(
      Symbols::getKernelDataStart() / 0x1000,
      (Symbols::getKernelDataStart() +
       Symbols::getKernelVBssEnd() - Symbols::getKernelVDataStart()) / 0x1000);

  // mapping stack
  mapRangeTo(
      Symbols::getStackBase() - 0x4000,
      Symbols::getStackBase(),
      0x800000 + 0x200000 - 0x4000,
      ATTR_RW);
  Memory::setRangeUsed(
      (0x800000 + 0x200000 - 0x4000) / 0x1000,
      0x4000 / 0x1000);

  // mapping page heap
  mapRangeTo(
      Symbols::getPageHeapBase(),
      Symbols::getPageHeapBase() + 32*0x1000,
      0xa00000,
      ATTR_RW);
  Memory::setRangeUsed(
      0xa00000 / 0x1000,
      (0xa00000 + 32*0x1000) / 0x1000);

  // mapping heap
  mapRangeTo(
      Symbols::getHeapBase(),
      Symbols::getHeapBase() + 0x200000,
      0xc00000,
      ATTR_RW);
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

void PageDirectory::mapPageTo(void* vaddr, uintptr_t ipage, uint8_t attributes)
{
  assert(g_pagingReady);

  _mapPageTo(vaddr, ipage, attributes);

  // refill pool if needed
  PageHeap::get().refillPool();
}

void PageDirectory::_mapPageTo(void* vaddr, page_t ipage, uint8_t attributes)
{
  if (reinterpret_cast<uintptr_t>(vaddr) < 0xffffffffc0000000 &&
      !(attributes & ATTR_PUBLIC) &&
      reinterpret_cast<uintptr_t>(vaddr) != 0xb8000)
    PANIC("Mapping private page in user space");
  if (reinterpret_cast<uintptr_t>(vaddr) > 0xffffffffc0000000 &&
      (attributes & ATTR_PUBLIC))
    PANIC("Mapping public page in kernel space");

  switch (attributes)
  {
#define CASE(n) \
  case n: mapPageToF(vaddr, ipage, AttributeSetter<n>); break;
    CASE(0x0)
    CASE(0x1)
    CASE(0x2)
    CASE(0x3)
#undef CASE
  default:
    PANIC("Invalid page attributes parameter");
  }
}

template <typename F>
void PageDirectory::mapPageToF(void* vaddr, page_t ipage, const F& f)
{
  Degf("Mapping %p to %x", vaddr, ipage << BASE_SHIFT);

  // map intermediate pages with all rights and limit permissions only on last
  // level
  PageTableEntry* page = m_manager->getPage(vaddr, AttributeSetter<0x3>);
  assert(!page->p && "Page already mapped");
  page->p = true;
  page->base = ipage;
  f(*page);
}

void PageDirectory::mapAddrTo(void* vaddr, physaddr_t ipaddr,
    uint8_t attributes)
{
  _mapPageTo(vaddr, ipaddr >> BASE_SHIFT, attributes);
}

void PageDirectory::mapRangeTo(void* vvastart, void* vvaend, physaddr_t pastart,
    uint8_t attributes)
{
  char* vastart = static_cast<char*>(vvastart);
  char* vaend = static_cast<char*>(vvaend);

  while (vastart < vaend)
  {
    mapAddrTo(vastart, pastart, attributes);

    vastart += 0x1000;
    pastart += 0x1000;
  }
}

void PageDirectory::mapPage(void* vaddr, uint8_t attributes, physaddr_t* paddr)
{
  assert(g_pagingReady);

  page_t page = Memory::getFreePage();

  assert(page != INVALID_PAGE);

  mapPageTo(vaddr, page, attributes);
  if (paddr)
    *paddr = page * 0x1000;
}

uintptr_t PageDirectory::unmapPage(void* vaddr)
{
  assert(g_pagingReady);

  PageTableEntry* page = m_manager->getPage(vaddr);
  assert(page && page->p && "Unmapping page that was not mapped");
  page->p = false;

  return page->base << BASE_SHIFT;
}

bool PageDirectory::isPageMapped(void* vaddr)
{
  assert(g_pagingReady);

  PageTableEntry* page = m_manager->getPage(vaddr);
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
