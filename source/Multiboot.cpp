#include "Multiboot.hpp"
#include "Util.hpp"
#include "Memory.hpp"
#include "Debug.hpp"
#include "Symbols.hpp"
#include "PageDirectory.hpp"

void MultibootLoader::handle(void* vmboot)
{
  //TagsHeader* header = reinterpret_cast<TagsHeader*>(vmboot);

  char* mboot = reinterpret_cast<char*>(vmboot);
  Tag* ptr = reinterpret_cast<Tag*>(mboot + sizeof(TagsHeader));

  while (ptr)
  {
    ptr = handleTag(reinterpret_cast<Tag*>(ptr));
  }
}

MultibootLoader::Tag* MultibootLoader::handleTag(Tag* tag)
{
  switch (tag->type)
  {
    case 0:
      return nullptr;
    case 6: // memory map
      handleMemoryMap(reinterpret_cast<MemoryMap*>(tag));
      break;
    case 3:
      handleModule(reinterpret_cast<Module*>(tag));
      break;
  }

  return ptrAlignSup(ptrAdd(tag, tag->size), 8);
}

void MultibootLoader::handleModule(Module* mod)
{
  const char* name = reinterpret_cast<char*>(mod) + sizeof(Module);

  char* curPtr = static_cast<char*>(Symbols::getStackBase());
  for (uint64_t page = mod->mod_start / 0x1000,
      lastPage = page + (mod->mod_end + 0xFFF) / 0x1000;
      page < lastPage;
      ++page, curPtr += 0x1000)
    PageDirectory::getKernelDirectory()->mapPageTo(curPtr, page);
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
