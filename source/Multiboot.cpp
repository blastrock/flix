#include "Multiboot.hpp"
#include "Util.hpp"
#include "Memory.hpp"
#include "Debug.hpp"
#include <iomanip>

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
  }

  return ptrAlignSup(ptrAdd(tag, tag->size), 8);
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
  fDeg() << std::hex <<
    "[" << std::setw(16) << std::setfill('0') << entry->base_addr << "-" <<
    std::setw(16) << std::setfill('0') <<
    entry->base_addr + entry->length << "] length " <<
    entry->length << " type " << entry->type;

  // if the segment is free to use, skip it
  if (entry->type == 1)
    return;

  for (uint64_t page = entry->base_addr / 0x1000,
      lastPage = page + entry->length / 0x1000;
      page < lastPage;
      ++page)
    Memory::setPageUsed(page);
}
