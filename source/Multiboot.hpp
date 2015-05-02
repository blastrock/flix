#ifndef MULTIBOOT_HPP
#define MULTIBOOT_HPP

#include <cstdint>

class MultibootLoader
{
  public:
    void handle(void* mboot)
    {
      handle(mboot, false);
    }
    void prepareMemory(void* mboot)
    {
      handle(mboot, true);
    }

  private:
    struct TagsHeader
    {
      uint32_t total_size;
      ///< Is 0 and must be ignored by the OS
      uint32_t reserved;
    } __attribute__((packed));

    struct Tag
    {
      uint32_t type;
      uint32_t size;
      // data
    } __attribute__((packed));

    struct MemoryMap
    {
      uint32_t type;
      uint32_t size;
      uint32_t entry_size;
      uint32_t entry_version;
      // entries
    } __attribute__((packed));

    struct MemoryMapEntry
    {
      uint64_t base_addr;
      uint64_t length;
      uint32_t type;
      ///< Is 0 and must be ignored by the OS
      uint32_t reserved;
    } __attribute__((packed));

    struct Module
    {
      uint32_t type;
      uint32_t size;
      uint32_t mod_start;
      uint32_t mod_end;
    } __attribute__((packed));

    void handle(void* vmboot, bool mem);

    void handleTag(Tag* tag);
    void handleModule(Module* mod);
    void handleMemoryMap(MemoryMap* map);
    void handleMemoryMapEntry(MemoryMapEntry* entry);

    bool moduleRead;
};

#endif /* MULTIBOOT_HPP */
