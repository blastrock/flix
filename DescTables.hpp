#ifndef DESC_TABLES_HPP
#define DESC_TABLES_HPP

#include "cstdint"

class DescTables
{
  public:
    static void init();

  private:
    // This structure contains the value of one GDT entry.
    // We use the attribute 'packed' to tell GCC not to change
    // any of the alignment in the structure.
    struct GdtEntry
    {
      uint16_t    limit_low;           // The lower 16 bits of the limit.
      uint16_t    base_low;            // The lower 16 bits of the base.
      uint8_t     base_middle;         // The next 8 bits of the base.
      uint8_t     access;              // Access flags, determine what ring this segment can be used in.
      uint8_t     granularity;
      uint8_t     base_high;           // The last 8 bits of the base.
    } __attribute__((packed));

    struct GdtPtr
    {
      uint16_t    limit;               // The upper 16 bits of all selector limits.
      uint32_t    base;                // The address of the first gdt_entry_t struct.
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

    static GdtEntry g_gdt_entries[5];
    static GdtPtr   g_gdt_ptr;
    static IdtEntry g_idt_entries[256];
    static IdtPtr   g_idt_ptr;

    static void initGdt();
    static void gdt_set_gate(uint32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
    static void initIdt();
    static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
};

#endif /* DESC_TABLES_HPP */
