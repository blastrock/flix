#ifndef DESC_TABLES_HPP
#define DESC_TABLES_HPP

#include <cstdint>

class DescTables
{
  public:
    static void init();

  private:
    struct GdtPtr
    {
      uint16_t limit;
      void* base;
    } __attribute__((packed));

    struct IdtEntry
    {
      uint16_t targetLow;
      uint16_t targetSelector;
      uint16_t flags; // should be 0x8E00
      uint16_t targetMid;
      uint32_t targetHigh;
      uint32_t reserved;
    } __attribute__((packed));

    static uint64_t g_gdtEntries[5];
    static GdtPtr g_gdtPtr;
    static IdtEntry g_idtEntries[256];
    static GdtPtr g_idtPtr;

    static void initGdt();
    static void initIdt();
    static void commitGdt(void* gdt);
    static void commitIdt(void* idt);
    static IdtEntry makeIdtGate(void* offset, uint16_t selector);
};

#endif /* DESC_TABLES_HPP */
