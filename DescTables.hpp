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

    // A struct describing an interrupt gate.
    struct IdtEntry
    {
      uint16_t    base_lo;             // The lower 16 bits of the address to jump to when this interrupt fires.
      uint16_t    sel;                 // Kernel segment selector.
      uint8_t     always0;             // This must always be zero.
      uint8_t     flags;               // More flags. See documentation.
      uint16_t    base_hi;             // The upper 16 bits of the address to jump to.
    } __attribute__((packed));

    // A struct describing a pointer to an array of interrupt handlers.
    // This is in a format suitable for giving to 'lidt'.
    struct IdtPtr
    {
      uint16_t    limit;
      uint32_t    base;                // The address of the first element in our idt_entry_t array.
    } __attribute__((packed));

    static uint64_t g_gdtEntries[5];
    static GdtPtr g_gdtPtr;
    static IdtEntry g_idt_entries[256];
    static IdtPtr   g_idt_ptr;

    static void initGdt();
    static void commitGdt(void* gdt);
    static void initIdt();
    static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
};

#endif /* DESC_TABLES_HPP */
