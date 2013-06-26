#include "Multiboot.hpp"
#include "Util.hpp"
#include "Memory.hpp"
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

  //XXX put this back
  //while (entry < end)
  //{
  //  handleMemoryMapEntry(entry);

  //  entry = ptrAdd(entry, map->entry_size);
  //}
  MemoryMapEntry e;
  e = {0, 0x9FC00, 1}; handleMemoryMapEntry(&e);
  e = {0x9fc00, 0x400, 2}; handleMemoryMapEntry(&e);
  e = {0xF0000, 0x10000, 2}; handleMemoryMapEntry(&e);
  e = {0x100000, 0x7efe000, 1}; handleMemoryMapEntry(&e);
  e = {0x7ffe000, 0x2000, 2}; handleMemoryMapEntry(&e);
  e = {0xfeffc000, 0x4000, 2}; handleMemoryMapEntry(&e);
  e = {0xfffc0000, 0x100000000, 2}; handleMemoryMapEntry(&e);
}

void MultibootLoader::handleMemoryMapEntry(MemoryMapEntry* entry)
{
  fDeg() << std::hex << "[" << entry->base_addr << "-" <<
    entry->base_addr + entry->length << "] length " << entry->length <<
    " type " << entry->type;

  // if the segment is free to use, skip it
  if (entry->type == 1)
    return;

  for (uint64_t page = entry->base_addr / 0x1000,
      lastPage = page + entry->length / 0x1000;
      page < lastPage;
      ++page)
    Memory::setPageUsed(page);
}
