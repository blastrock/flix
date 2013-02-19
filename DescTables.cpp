#include "DescTables.hpp"
#include <cstring>
#include "io.hpp"

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
idt_isr(32)
idt_isr(33)
idt_isr(34)
idt_isr(35)
idt_isr(36)
idt_isr(37)
idt_isr(38)
idt_isr(39)
idt_isr(40)
idt_isr(41)
idt_isr(42)
idt_isr(43)
idt_isr(44)
idt_isr(45)
idt_isr(46)
idt_isr(47)
#undef idt_isr

DescTables::GdtEntry DescTables::g_gdt_entries[5];
DescTables::GdtPtr   DescTables::g_gdt_ptr;
DescTables::IdtEntry DescTables::g_idt_entries[256];
DescTables::IdtPtr   DescTables::g_idt_ptr;

extern "C" void gdt_set(void* addr);
extern "C" void idt_set(void* addr);

void DescTables::init()
{
  initIdt();
  initGdt();
}

void DescTables::initGdt()
{
  g_gdt_ptr.limit = (sizeof(GdtEntry) * 5) - 1;
  g_gdt_ptr.base  = (uint32_t)&g_gdt_entries;

  g_gdt_entries[0] = {0, 0, 0, 0, 0, 0};
  g_gdt_entries[1] = {0xFFFF, 0x0000, 0x00, 0x9A, 0xCF, 0x00};
  g_gdt_entries[2] = {0xFFFF, 0x0000, 0x00, 0x92, 0xCF, 0x00};
  g_gdt_entries[3] = {0xFFFF, 0x0000, 0x00, 0xFA, 0xCF, 0x00};
  g_gdt_entries[4] = {0xFFFF, 0x0000, 0x00, 0xF2, 0xCF, 0x00};

  gdt_set(&g_gdt_ptr);
}

void DescTables::initIdt()
{
  g_idt_ptr.limit = sizeof(IdtEntry) * 256 -1;
  g_idt_ptr.base  = (uint32_t)&g_idt_entries;

  std::memset(&g_idt_entries, 0, sizeof(IdtEntry)*256);

  // Remap the irq table.
  // init
  io::outb(0x20, 0x11);
  io::outb(0xA0, 0x11);
  // offsets
  io::outb(0x21, 0x20);
  io::outb(0xA1, 0x28);
  // connections
  io::outb(0x21, 0x04);
  io::outb(0xA1, 0x02);
  // environment
  io::outb(0x21, 0x01);
  io::outb(0xA1, 0x01);
  // reset masks
  io::outb(0x21, 0x0);
  io::outb(0xA1, 0x0);

#define idt_set(num) \
  idt_set_gate(num, (uint32_t)isr##num, 0x08, 0x8E);
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
  idt_set(32)
  idt_set(33)
  idt_set(34)
  idt_set(35)
  idt_set(36)
  idt_set(37)
  idt_set(38)
  idt_set(39)
  idt_set(40)
  idt_set(41)
  idt_set(42)
  idt_set(43)
  idt_set(44)
  idt_set(45)
  idt_set(46)
  idt_set(47)
#undef idt_set

  idt_set(&g_idt_ptr);
}

void DescTables::idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
  g_idt_entries[num].base_lo = base & 0xFFFF;
  g_idt_entries[num].base_hi = (base >> 16) & 0xFFFF;

  g_idt_entries[num].sel     = sel;
  g_idt_entries[num].always0 = 0;
  // We must uncomment the OR below when we get to using user-mode.
  // It sets the interrupt gate's privilege level to 3.
  g_idt_entries[num].flags   = flags /* | 0x60 */;
}
