#ifndef DESC_TABLES_HPP
#define DESC_TABLES_HPP

#include "inttypes.hpp"

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
      u16    limit_low;           // The lower 16 bits of the limit.
      u16    base_low;            // The lower 16 bits of the base.
      u8     base_middle;         // The next 8 bits of the base.
      u8     access;              // Access flags, determine what ring this segment can be used in.
      u8     granularity;
      u8     base_high;           // The last 8 bits of the base.
    } __attribute__((packed));

    struct GdtPtr
    {
      u16    limit;               // The upper 16 bits of all selector limits.
      u32    base;                // The address of the first gdt_entry_t struct.
    } __attribute__((packed));

    // A struct describing an interrupt gate.
    struct IdtEntry
    {
      u16    base_lo;             // The lower 16 bits of the address to jump to when this interrupt fires.
      u16    sel;                 // Kernel segment selector.
      u8     always0;             // This must always be zero.
      u8     flags;               // More flags. See documentation.
      u16    base_hi;             // The upper 16 bits of the address to jump to.
    } __attribute__((packed));

    // A struct describing a pointer to an array of interrupt handlers.
    // This is in a format suitable for giving to 'lidt'.
    struct IdtPtr
    {
      u16    limit;
      u32    base;                // The address of the first element in our idt_entry_t array.
    } __attribute__((packed));

    static GdtEntry g_gdt_entries[5];
    static GdtPtr   g_gdt_ptr;
    static IdtEntry g_idt_entries[256];
    static IdtPtr   g_idt_ptr;

    static void initGdt();
    static void gdt_set_gate(u32 num, u32 base, u32 limit, u8 access, u8 gran);
    static void initIdt();
    static void idt_set_gate(u8 num, u32 base, u16 sel, u8 flags);
};

#endif /* DESC_TABLES_HPP */
