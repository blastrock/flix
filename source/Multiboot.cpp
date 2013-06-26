#include "Multiboot.hpp"
#include "Util.hpp"
#include "Debug.hpp"

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
  fDeg() << "handling memory tag";

  MemoryMapEntry* entry = reinterpret_cast<MemoryMapEntry*>(
      reinterpret_cast<char*>(map)+sizeof(MemoryMap));
  MemoryMapEntry* end = ptrAdd(entry, map->size);

  while (entry < end)
  {
    handleMemoryMapEntry(entry);

    entry = ptrAdd(entry, map->entry_size);
  }
}

void MultibootLoader::handleMemoryMapEntry(MemoryMapEntry* entry)
{
}
