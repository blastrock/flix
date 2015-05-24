#ifndef DESC_TABLES_HPP
#define DESC_TABLES_HPP

#include <cstdint>

class DescTables
{
  public:
    static void init();
    static void initTr();

  private:
    // TODO move this stuff in the cpp file
    struct GdtPtr
    {
      uint16_t limit;
      const void* base;
    } __attribute__((packed));

    struct IdtEntry
    {
      uint16_t targetLow;
      uint16_t targetSelector;
      uint16_t flags;
      uint16_t targetMid;
      uint32_t targetHigh;
      uint32_t reserved;
    } __attribute__((packed));

    static uint64_t g_gdtEntries[7];
    static const GdtPtr g_gdtPtr;
    static IdtEntry g_idtEntries[256];
    static const GdtPtr g_idtPtr;

    static void initGdt();
    static void initIdt();
    static void commitGdt(const void* gdt);
    static void commitIdt(const void* idt);
    static IdtEntry makeIdtGate(void* offset, uint16_t selector, bool pub);
};

#endif /* DESC_TABLES_HPP */
