#include "Multiboot.hpp"
#include "Util.hpp"
#include "Memory.hpp"
#include "Debug.hpp"
#include "Symbols.hpp"
#include "PageDirectory.hpp"
#include "Cpio.hpp"

// memory tag must be handled very early during boot and it's hard to handle
// other tags without making memory allocations, so memory tag handling is a
// special case

void MultibootLoader::handle(void* vmboot, bool mem)
{
  //TagsHeader* header = reinterpret_cast<TagsHeader*>(vmboot);

  char* mboot = reinterpret_cast<char*>(vmboot);

  for (Tag* tag = reinterpret_cast<Tag*>(mboot + sizeof(TagsHeader));
      tag->type != 0;
      tag = ptrAlignSup(ptrAdd(tag, tag->size), 8))
  {
    if (mem)
    {
      if (tag->type != 6)
        continue;
      else
      {
        handleMemoryMap(reinterpret_cast<MemoryMap*>(tag));
        return;
      }
    }
    else
      handleTag(tag);
  }

  if (mem)
    PANIC("Memory tag not found in multiboot information");
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

void MultibootLoader::handleModule(Module* mod)
{
  if (moduleRead)
    PANIC("More than one module specified on boot");

  //const char* name = reinterpret_cast<char*>(mod) + sizeof(Module);

  char* basePtr = static_cast<char*>(Symbols::getStackBase());
  char* curPtr = basePtr;
  for (uint64_t page = mod->mod_start / 0x1000,
      lastPage = page + (mod->mod_end + 0xFFF) / 0x1000;
      page < lastPage;
      ++page, curPtr += 0x1000)
    PageDirectory::getKernelDirectory()->mapPageTo(curPtr, page);

  fs::setRoot(readArchive(basePtr));

  moduleRead = true;
}

void MultibootLoader::handleMemoryMap(MemoryMap* map)
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

  for (uint64_t page = entry->base_addr / 0x1000,
      // TODO shouldn't we ceil() this?
      lastPage = page + entry->length / 0x1000;
      page < lastPage;
      ++page)
    Memory::setPageUsed(page);
}
