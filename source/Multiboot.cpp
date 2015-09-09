#include "Multiboot.hpp"
#include "Util.hpp"
#include "Memory.hpp"
#include "Debug.hpp"
#include "Symbols.hpp"
#include "PageDirectory.hpp"
#include "Cpio.hpp"

XLL_LOG_CATEGORY("boot/multiboot");

// memory tag must be handled very early during boot and it's hard to handle
// other tags without making memory allocations, so memory tag handling is a
// special case

void MultibootLoader::handle(void* vmboot, bool mem)
{
  //TagsHeader* header = reinterpret_cast<TagsHeader*>(vmboot);

  char* mboot = reinterpret_cast<char*>(vmboot);
  bool memFound = false;

  for (Tag* tag = reinterpret_cast<Tag*>(mboot + sizeof(TagsHeader));
      tag->type != 0;
      tag = ptrAlignSup(ptrAdd(tag, tag->size), 8))
    if (mem)
    {
      if (tag->type == 6)
        memFound = true;
      prehandleTag(tag);
    }
    else
      handleTag(tag);

  if (mem && !memFound)
    PANIC("Memory tag not found in multiboot information");
}

void MultibootLoader::prehandleTag(Tag* tag)
{
  switch (tag->type)
  {
    case 3:
      prehandleModule(reinterpret_cast<Module*>(tag));
      break;
    case 6:
      prehandleMemoryMap(reinterpret_cast<MemoryMap*>(tag));
      break;
  }
}

void MultibootLoader::handleTag(Tag* tag)
{
  switch (tag->type)
  {
    case 3:
      handleModule(reinterpret_cast<Module*>(tag));
      break;
  }
}

template <typename F>
char* MultibootLoader::handleModule(Module* mod, const F& cb)
{
  //const char* name = reinterpret_cast<char*>(mod) + sizeof(Module);

  char* basePtr = static_cast<char*>(Symbols::getStackBase());
  char* curPtr = basePtr;
  for (physaddr_t paddr = mod->mod_start,
      lastPAddr = mod->mod_end;
      paddr < lastPAddr;
      paddr += PAGE_SIZE, curPtr += PAGE_SIZE)
    cb(curPtr, paddr);

  return basePtr;
}

void MultibootLoader::prehandleModule(Module* mod)
{
  xDeb("Reserved memory for module: %x-%x", mod->mod_start, mod->mod_end);
  handleModule(mod,
      [](auto, auto paddr) {
        Memory::setPageUsed(paddr / PAGE_SIZE);
      });
}

void MultibootLoader::handleModule(Module* mod)
{
  if (_moduleRead)
    PANIC("More than one module specified on boot");

  xDeb("Mapping module %x-%x", mod->mod_start, mod->mod_end);
  char* basePtr = handleModule(mod,
      [](auto curPtr, auto paddr) {
        PageDirectory::getKernelDirectory()->mapPageTo(curPtr, paddr, 0);
      });

  fs::setRoot(readArchive(basePtr));

  _moduleRead = true;
}

void MultibootLoader::prehandleMemoryMap(MemoryMap* map)
{
  MemoryMapEntry* entry = reinterpret_cast<MemoryMapEntry*>(
      reinterpret_cast<char*>(map)+sizeof(MemoryMap));
  MemoryMapEntry* end = ptrAdd(entry, map->size);

  while (ptrAdd(entry, map->entry_size) < end)
  {
    handleMemoryMapEntry(entry);

    entry = ptrAdd(entry, map->entry_size);
  }
}

void MultibootLoader::handleMemoryMapEntry(MemoryMapEntry* entry)
{
  // if the segment is free to use, skip it
  if (entry->type == 1)
    return;

  xDeb("Reserved memory chunk: %x-%x", entry->base_addr,
      entry->base_addr + entry->length);

  for (uint64_t page = entry->base_addr / 0x1000,
      lastPage = page + (entry->length + 0xFFF) / 0x1000;
      page < lastPage;
      ++page)
    Memory::setPageUsed(page);
}
