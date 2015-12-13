#include "PageDirectory.hpp"
#include "KHeap.hpp"
#include "Symbols.hpp"
#include "Memory.hpp"
#include "Util.hpp"
#include "Debug.hpp"

XLL_LOG_CATEGORY("core/memory/pagedirectory");

PageDirectory* PageDirectory::g_kernelDirectory = nullptr;

PageDirectory* PageDirectory::initKernelDirectory()
{
  // enable NXE
  Cpu::writeMsr(Cpu::MSR_EFER, Cpu::readMsr(Cpu::MSR_EFER) | 1 << 11);

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
  if (Attr & PageDirectory::ATTR_NOEXEC)
    e.nx = true;
  if (Attr & PageDirectory::ATTR_DEFER)
    e.p = false;
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

  xDeb("Mapping VGA");
  _mapPageTo(Symbols::getKernelVTextStart() + 0x01000000, 0xB8000, ATTR_RW);
  Memory::setPageUsed(0xB8000 / 0x1000);

#define MAP_RANGE(name, from, to, phys, attr)         \
  xDeb("Mapping " name " (size: %x)", (to) - (from)); \
  mapRangeTo((from), (to), (phys), (attr));           \
  Memory::setRangeUsed((phys) / 0x1000, ((phys) + ((to) - (from))) / 0x1000);

#define MAP_RANGE_SYM(name, from, to, phys, attr)                       \
  MAP_RANGE(name, Symbols::getKernel##from(), Symbols::getKernel##to(), \
      Symbols::getKernel##phys(), attr)

  MAP_RANGE_SYM(".text", VTextStart, VTextEnd, TextStart, 0)
  MAP_RANGE_SYM(".text", VRodataStart, VRodataEnd, RodataStart, ATTR_NOEXEC)
  MAP_RANGE_SYM(".data and .bss", VDataStart, VBssEnd, DataStart,
      ATTR_NOEXEC | ATTR_RW)

  uintptr_t addr = reinterpret_cast<uintptr_t>(Symbols::getKernelBssEnd());
  addr = intAlignSup(addr, 0x200000);

  MAP_RANGE("kernel stack",
      Symbols::getStackBase() - 0x4000,
      Symbols::getStackBase(),
      addr + 0x200000 - 0x4000,
      ATTR_NOEXEC | ATTR_RW);

  addr += 0x200000;

  MAP_RANGE("page heap",
      Symbols::getPageHeapBase(),
      Symbols::getPageHeapBase() + 32*0x1000,
      addr,
      ATTR_NOEXEC | ATTR_RW);

  addr += 0x200000;

  MAP_RANGE("kernel heap",
      Symbols::getHeapBase(),
      Symbols::getHeapBase() + 0x200000,
      addr,
      ATTR_NOEXEC | ATTR_RW);

#undef MAP_RANGE_SYM
#undef MAP_RANGE
}

static PageDirectory* g_currentPageDirectory = 0;

void PageDirectory::use()
{
  xDeb("Changing pagetable to %x", m_directory.value);

#ifndef NDEBUG
  if (g_pagingReady)
  {
    int var;
    // check that we keep the same stack before and after the change
    assert((!g_pagingReady || getCurrent()->resolve(&var) == resolve(&var)) &&
        "Changing page directory would mess up the stack!");
  }
  else
    g_pagingReady = true;
#endif

  asm volatile("mov %0, %%cr3":: "r"(m_directory.value));
  g_currentPageDirectory = this;
}

PageDirectory* PageDirectory::getCurrent()
{
  return g_currentPageDirectory;
}

void PageDirectory::mapPageTo(void* vaddr, physaddr_t paddr, uint8_t attributes)
{
  assert(g_pagingReady);

  _mapPageTo(vaddr, paddr, attributes);

  // refill pool if needed
  getPdPageHeap().refillPool();
}

void PageDirectory::_mapPageTo(void* vaddr, physaddr_t paddr, uint8_t attributes)
{
  // we consider everything above 0xffffffff00000000 is kernel space, but
  // everything up to 0xffffffffc0000000 is process-specific
  if (reinterpret_cast<uintptr_t>(vaddr) < 0xffffffff00000000 &&
      !(attributes & ATTR_PUBLIC))
    PANIC("Mapping private page in user space");
  if (reinterpret_cast<uintptr_t>(vaddr) > 0xffffffffc0000000 &&
      (attributes & ATTR_PUBLIC))
    PANIC("Mapping public page in kernel space");

  switch (attributes)
  {
#define CASE(n) \
  case n: mapPageToF(vaddr, paddr, AttributeSetter<n>); break;
    CASE(0x0)
    CASE(0x1)
    CASE(0x2)
    CASE(0x3)
    CASE(0x4)
    CASE(0x5)
    CASE(0x6)
    CASE(0x7)
    CASE(0x8)
    CASE(0x9)
    CASE(0xa)
    CASE(0xb)
    CASE(0xc)
    CASE(0xd)
    CASE(0xe)
    CASE(0xf)
#undef CASE
  default:
    PANIC("Invalid page attributes parameter");
  }
}

template <typename F>
void PageDirectory::mapPageToF(void* vaddr, physaddr_t paddr, const F& f)
{
  xDeb("Mapping %p to %x", vaddr, paddr);

  // map intermediate pages with all rights and limit permissions only on last
  // level
  PageTableEntry* page = m_manager->getPage(vaddr, AttributeSetter<0x3>);
  assert(!page->p && "Page already mapped");
  page->p = true;
  page->base = paddr >> BASE_SHIFT;
  f(*page);
}

void PageDirectory::mapRangeTo(void* vvastart, void* vvaend, physaddr_t pastart,
    uint8_t attributes)
{
  char* vastart = static_cast<char*>(vvastart);
  char* vaend = static_cast<char*>(vvaend);

  while (vastart < vaend)
  {
    _mapPageTo(vastart, pastart, attributes);

    vastart += 0x1000;
    pastart += 0x1000;
  }
}

void PageDirectory::mapRange(void* vvastart, void* vvaend, uint8_t attributes)
{
  for (char* vastart = static_cast<char*>(vvastart),
             * vaend = static_cast<char*>(vvaend);
       vastart < vaend;
       vastart += PAGE_SIZE)
  {
    mapPage(vastart, attributes);
  }
}

void PageDirectory::mapPage(void* vaddr, uint8_t attributes, physaddr_t* paddr)
{
  assert(g_pagingReady);

  physaddr_t target;

  if (attributes & ATTR_DEFER)
    target = INVALID_PHYS;
  else
  {
    page_t page = Memory::getFreePage();
    assert(page != INVALID_PAGE);
    target = page * PAGE_SIZE;
  }

  mapPageTo(vaddr, target, attributes);
  if (paddr)
    *paddr = target;
}

physaddr_t PageDirectory::unmapPage(void* vaddr)
{
  assert(g_pagingReady);

  PageTableEntry* page = m_manager->getPage(vaddr);
  assert(page && page->p && "Unmapping page that was not mapped");
  page->p = false;
  const auto base = page->base;
  page->base = 0;

  return base << BASE_SHIFT;
}

physaddr_t PageDirectory::resolve(void* vaddr)
{
  assert(g_pagingReady);

  PageTableEntry* page = m_manager->getPage(vaddr);
  if (!page || !page->p)
    return INVALID_PHYS;

  return page->base << BASE_SHIFT;
}

bool PageDirectory::handleFault(void* vaddr)
{
  PageTableEntry* entry = m_manager->getPage(vaddr);
  if (!entry || entry->p || entry->base != INVALID_PAGE)
  {
    if (entry)
      xDeb("Not a deferred allocation %s %x", entry->p, entry->base);
    else
      xDeb("Not a deferred allocation (no entry)");
    return false;
  }

  page_t page = Memory::getFreePage();
  entry->p = true;
  entry->base = page;
  xDeb("Handled deferred allocation, mapped %p to %x",
      vaddr, page << BASE_SHIFT);

  return true;
}

bool PageDirectory::isPageMapped(void* vaddr)
{
  assert(g_pagingReady);

  PageTableEntry* page = m_manager->getPage(vaddr);
  return page && page->p;
}

std::optional<physaddr_t> PageDirectory::getPhysicalAddress(void* vaddr)
{
  assert(g_pagingReady);

  PageTableEntry* page = m_manager->getPage(vaddr);
  if (!page && !page->p)
    return std::nullopt;

  return page->base << BASE_SHIFT;
}

// TODO make something to invalidate a single page (invlpg)
void PageDirectory::flushTlb()
{
  asm volatile(
      "mov %%cr3, %%rax\n"
      "mov %%rax, %%cr3\n"
      :::"rax");
}
