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

// This function reuses the kernel's page directory for the upper adresses
void PageDirectory::mapKernel()
{
  // TODO this function should not initialize, or its name should be changed
  // TODO this code is copy pasted from below
  std::pair<X86_64PageManager*, void*> pm = X86_64PageManager::makeNew();
  m_manager = pm.first;

  m_directory.value = 0;
  m_directory.bitfield.base = reinterpret_cast<uintptr_t>(pm.second) >> 12;

  auto* kern =
    getKernelDirectory()->m_manager->getEntry<2>(0xffffffffc0000000, false);

  assert(!isInvalid(kern));

  *m_manager->getEntry<2>(0xffffffffc0000000, true) = *kern;
}

void PageDirectory::initWithDefaultPaging()
{
  std::pair<X86_64PageManager*, void*> pm = X86_64PageManager::makeNew();
  m_manager = pm.first;

  m_directory.value = 0;
  m_directory.bitfield.base = reinterpret_cast<uintptr_t>(pm.second) >> 12;

  // map VGA
  {
    PageTableEntry* page = m_manager->getPage(0xB8000, true);
    page->p = true;
    page->us = true;
    page->rw = true;
    page->base = 0xB8000 >> 12;
  }

  // mapping .text
  uintptr_t vcur = reinterpret_cast<uintptr_t>(Symbols::getKernelVTextStart());
  uintptr_t cur = reinterpret_cast<uintptr_t>(Symbols::getKernelTextStart());
  uintptr_t end = reinterpret_cast<uintptr_t>(Symbols::getKernelBssEnd());
  while (cur < end)
  {
    PageTableEntry* page = m_manager->getPage(vcur, true);
    page->p = true;
    page->us = true;
    page->rw = true;
    page->base = cur >> 12;

    cur += 0x1000;
    vcur += 0x1000;
  }

  // mapping stack
  cur = 0x800000 + 0x200000 - 0x4000;
  end = cur + 0x4000;
  vcur = reinterpret_cast<uintptr_t>(Symbols::getStackBase()) - 0x4000;
  while (cur < end)
  {
    PageTableEntry* page = m_manager->getPage(vcur, true);
    page->p = true;
    page->us = true;
    page->rw = true;
    page->base = cur >> 12;

    cur += 0x1000;
    vcur += 0x1000;
  }

  // mapping page heap
  vcur = reinterpret_cast<uintptr_t>(Symbols::getPageHeapBase());
  cur = 0xa00000;
  end = cur + 0x200000;
  while (cur < end)
  {
    PageTableEntry* page = m_manager->getPage(vcur, true);
    page->p = true;
    page->us = true;
    page->rw = true;
    page->base = cur >> 12;

    cur += 0x1000;
    vcur += 0x1000;
  }

  // mapping heap
  vcur = reinterpret_cast<uintptr_t>(Symbols::getHeapBase());
  cur = 0xc00000;
  end = cur + 0x200000;
  while (cur < end)
  {
    PageTableEntry* page = m_manager->getPage(vcur, true);
    page->p = true;
    page->us = true;
    page->rw = true;
    page->base = cur >> 12;

    cur += 0x1000;
    vcur += 0x1000;
  }
}

static PageDirectory* g_currentPageDirectory = 0;

void PageDirectory::use()
{
  Degf("changing pagetable to %x", m_directory.value);
  asm volatile("mov %0, %%cr3":: "r"(m_directory.value));
  g_currentPageDirectory = this;
}

PageDirectory* PageDirectory::getCurrent()
{
  return g_currentPageDirectory;
}

void PageDirectory::mapPageTo(void* vaddr, uintptr_t ipage)
{
  uintptr_t ivaddr = reinterpret_cast<uintptr_t>(vaddr);

  PageTableEntry* page = m_manager->getPage(ivaddr, true);
  assert(!page->p && "Page already mapped");
  // TODO move this, only system pages may be mapped like this, which is bad
  page->p = true;
  page->us = true;
  page->rw = true;
  page->base = ipage;
}

void PageDirectory::mapPage(void* vaddr, void** paddr)
{
  uintptr_t page = Memory::getFreePage();

  assert(page != static_cast<uintptr_t>(-1));

  mapPageTo(vaddr, page);
  if (paddr)
    *paddr = reinterpret_cast<void*>(page * 0x1000);
}

void PageDirectory::unmapPage(void* vaddr)
{
  uintptr_t ivaddr = reinterpret_cast<uintptr_t>(vaddr);

  PageTableEntry* page = m_manager->getPage(ivaddr, true);
  assert(page->p);
  page->p = false;

  Memory::setPageFree(ivaddr / 0x1000);
}

bool PageDirectory::isPageMapped(void* vaddr)
{
  uintptr_t ivaddr = reinterpret_cast<uintptr_t>(vaddr);
  PageTableEntry* page = m_manager->getPage(ivaddr, false);
  return isValid(page) && page->p;
}
