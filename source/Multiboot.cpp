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
  for (uint64_t page = mod->mod_start / 0x1000,
      lastPage = (mod->mod_end + 0xFFF) / 0x1000;
      page < lastPage;
      ++page, curPtr += 0x1000)
    cb(curPtr, page);

  return basePtr;
}

void MultibootLoader::prehandleModule(Module* mod)
{
  handleModule(mod,
      [](auto, auto page) {
        Memory::setPageUsed(page);
      });
}

void MultibootLoader::handleModule(Module* mod)
{
  if (_moduleRead)
    PANIC("More than one module specified on boot");

  char* basePtr = handleModule(mod,
      [](auto curPtr, auto page) {
        PageDirectory::getKernelDirectory()->mapPageTo(curPtr, page);
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

  for (uint64_t page = entry->base_addr / 0x1000,
      // TODO shouldn't we ceil() this?
      lastPage = page + entry->length / 0x1000;
      page < lastPage;
      ++page)
    Memory::setPageUsed(page);
}
