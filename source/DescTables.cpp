#include "DescTables.hpp"
#include <cstring>
#include "io.hpp"
#include "Debug.hpp"

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

// the GDT must not be marked const because the ltr instruction writes in there
static uint64_t g_gdtEntries[] = {
  // null descriptor
  0x0000000000000000,
  // code segment (ring0)
  0x0020980000000000,
  // data segment (ring0)
  0x0000920000000000,
  // code segment (ring3)
  0x0020F80000000000,
  // data segment (ring3)
  0x0000F20000000000,
  // task segment (128bits, LE)
  // points to the bottom of the kernel stack (0xffffffffd0000000 - 0x4000)
  0xCF0089FFC0000067,
  0x00000000FFFFFFFF,
};

static const GdtPtr g_gdtPtr = {
  sizeof(g_gdtEntries) - 1,
  g_gdtEntries
};
static IdtEntry g_idtEntries[256];
static const GdtPtr g_idtPtr = {
  sizeof(g_idtEntries) - 1,
  g_idtEntries
};

extern "C" void* intVectors[];

static void initGdt();
static void initIdt();
static void commitGdt(const void* gdt);
static void commitIdt(const void* idt);
static IdtEntry makeIdtGate(void* offset, uint16_t selector, bool pub);

void commitGdt(const void* gdt)
{
  asm volatile(
    "lgdt (%0)\n"

    "movw $0x10, %%ax\n"
    "movw %%ax, %%ds\n"
    "movw %%ax, %%es\n"
    "movw %%ax, %%fs\n"
    "movw %%ax, %%gs\n"
    "movw %%ax, %%ss\n"
    "pushq $0x08\n"
    "pushq $.myflush\n"
    "lretq\n"

  ".myflush:\n"
    :
    :"a"(gdt)
    :"%rax");
}

void commitIdt(const void* idt)
{
  asm volatile(
      "lidt (%0)"
      :
      :"a"(idt));
}

void DescTables::init()
{
  initGdt();
  initIdt();
}

void initGdt()
{
  commitGdt(&g_gdtPtr);
}

void initIdt()
{
  std::memset(&g_idtEntries, 0, sizeof(g_idtEntries));

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

  for (short i = 0; i < 48; ++i)
    g_idtEntries[i] = makeIdtGate(intVectors[i], 0x08, false);

  g_idtEntries[0x80] = makeIdtGate(intVectors[48], 0x08, true);

  commitIdt(&g_idtPtr);
}

void DescTables::initTr()
{
  asm volatile("ltr %0" : :"r"(static_cast<uint16_t>(0x28)));
}

// Make gate that points to code at offset. If pub is true, the gate is
// accessible from unpriviledged code
// TODO is the selector argument useful? it's always equal to 0x08
IdtEntry makeIdtGate(void* offset, uint16_t selector,
    bool pub)
{
  uint64_t ioff = reinterpret_cast<uint64_t>(offset);

  IdtEntry entry;
  entry.targetLow = ioff & 0xFFFF;
  entry.targetMid = (ioff >> 16) & 0xFFFF;
  entry.targetHigh = (ioff >> 32) & 0xFFFFFFFF;
  entry.targetSelector = selector;
  // present, DPL = 0, type = Interrupt, IST = 1
  entry.flags = 0x8E01;
  if (pub)
    entry.flags |= 0x6000;

  return entry;
}
