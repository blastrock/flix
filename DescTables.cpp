#include "DescTables.hpp"
#include "string.h"

#define idt_isr(num) \
  extern "C" void isr##num();
idt_isr(0)
idt_isr(1)
idt_isr(2)
idt_isr(3)
idt_isr(4)
idt_isr(5)
idt_isr(6)
idt_isr(7)
idt_isr(8)
idt_isr(9)
idt_isr(10)
idt_isr(11)
idt_isr(12)
idt_isr(13)
idt_isr(14)
idt_isr(15)
idt_isr(16)
idt_isr(17)
idt_isr(18)
idt_isr(19)
idt_isr(20)
idt_isr(21)
idt_isr(22)
idt_isr(23)
idt_isr(24)
idt_isr(25)
idt_isr(26)
idt_isr(27)
idt_isr(28)
idt_isr(29)
idt_isr(30)
idt_isr(31)
#undef idt_isr

DescTables::GdtEntry DescTables::g_gdt_entries[5];
DescTables::GdtPtr   DescTables::g_gdt_ptr;
DescTables::IdtEntry DescTables::g_idt_entries[256];
DescTables::IdtPtr   DescTables::g_idt_ptr;

extern "C" void gdt_set(void* addr);
extern "C" void idt_set(void* addr);

void DescTables::Init()
{
  InitIdt();
  InitGdt();
}

void DescTables::InitGdt()
{
  g_gdt_ptr.limit = (sizeof(GdtEntry) * 5) - 1;
  g_gdt_ptr.base  = (u32)&g_gdt_entries;

  gdt_set_gate(0, 0, 0, 0, 0);                // Null segment
  gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment
  gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment
  gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User mode code segment
  gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User mode data segment

  gdt_set(&g_gdt_ptr);
}

void DescTables::gdt_set_gate(u32 num, u32 base, u32 limit, u8 access, u8 gran)
{
  g_gdt_entries[num].base_low    = (base & 0xFFFF);
  g_gdt_entries[num].base_middle = (base >> 16) & 0xFF;
  g_gdt_entries[num].base_high   = (base >> 24) & 0xFF;

  g_gdt_entries[num].limit_low   = (limit & 0xFFFF);
  g_gdt_entries[num].granularity = (limit >> 16) & 0x0F;

  g_gdt_entries[num].granularity |= gran & 0xF0;
  g_gdt_entries[num].access      = access;
}

void DescTables::InitIdt()
{
  g_idt_ptr.limit = sizeof(IdtEntry) * 256 -1;
  g_idt_ptr.base  = (u32)&g_idt_entries;

  memset(&g_idt_entries, 0, sizeof(IdtEntry)*256);

#define idt_set(num) \
  idt_set_gate(num, (u32)isr##num, 0x08, 0x8E);
  idt_set(0)
  idt_set(1)
  idt_set(2)
  idt_set(3)
  idt_set(4)
  idt_set(5)
  idt_set(6)
  idt_set(7)
  idt_set(8)
  idt_set(9)
  idt_set(10)
  idt_set(11)
  idt_set(12)
  idt_set(13)
  idt_set(14)
  idt_set(15)
  idt_set(16)
  idt_set(17)
  idt_set(18)
  idt_set(19)
  idt_set(20)
  idt_set(21)
  idt_set(22)
  idt_set(23)
  idt_set(24)
  idt_set(25)
  idt_set(26)
  idt_set(27)
  idt_set(28)
  idt_set(29)
  idt_set(30)
  idt_set(31)
#undef idt_set

  idt_set(&g_idt_ptr);
}

void DescTables::idt_set_gate(u8 num, u32 base, u16 sel, u8 flags)
{
  g_idt_entries[num].base_lo = base & 0xFFFF;
  g_idt_entries[num].base_hi = (base >> 16) & 0xFFFF;

  g_idt_entries[num].sel     = sel;
  g_idt_entries[num].always0 = 0;
  // We must uncomment the OR below when we get to using user-mode.
  // It sets the interrupt gate's privilege level to 3.
  g_idt_entries[num].flags   = flags /* | 0x60 */;
}
