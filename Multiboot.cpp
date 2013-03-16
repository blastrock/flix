#include "Multiboot.hpp"
#include "Debug.hpp"

// TODO move these functions
template <typename T>
inline T* ptrAdd(T* ptr, uint64_t val)
{
  return reinterpret_cast<T*>(reinterpret_cast<char*>(ptr)+val);
}

template <typename T1, typename T2>
inline uint64_t ptrDiff(T1* ptr1, T2* ptr2)
{
  return reinterpret_cast<char*>(ptr1)-reinterpret_cast<char*>(ptr2);
}

/**
 * Aligns a pointer to val above bytes
 *
 * \example If called with 0x9 and 8, it will return 0x10.
 *
 * \warning val must be a power of two
 */
template <typename T>
inline T* ptrAlignSup(T* ptr, uint8_t val)
{
  return reinterpret_cast<T*>((reinterpret_cast<uint64_t>(ptr) + val-1)
      & ~(uint64_t)(val-1));
}

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

  while (entry < end)
  {
    handleMemoryMapEntry(entry);

    entry = ptrAdd(entry, map->entry_size);
  }
}

void MultibootLoader::handleMemoryMapEntry(MemoryMapEntry* entry)
{
  debug("base", entry->base_addr);
  debug("length", entry->length);
}
