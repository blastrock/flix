PHYSICAL_BASE = 0x100000;
VIRTUAL_BASE = 0xffffffff80000000;
PHYSICAL_HEAP = 0x400000;
VIRTUAL_HEAP = 0xffffffffc0000000;
SHIFT = VIRTUAL_BASE; /* - PHYS_BASE;*/

ENTRY(start)
SECTIONS
{
  . = PHYSICAL_BASE;

  .bootstrap :
  {
    _kernelStart = ABSOLUTE(.);
    *(.mbhdr)
    *(.bootstrap)
    . = ALIGN(0x1000);
    Pml4 = .;
    . += 0x1000;
    Pdpt = .;
    . += 0x1000;
    Pd = .;
    . += 0x1000;
    . += 0x1000;

    . += 0x1000;
    _stack = .;
  }

  . += SHIFT;

  .text ALIGN(0x1000) :
    AT(ADDR(.text) - SHIFT)
  {
    *(.text*)
    . = ALIGN(0x1000);
  }

  .data ALIGN(0x1000) :
    AT(ADDR(.data) - SHIFT)
  {
    *(.data*)
    *(.rodata*)
    *(.got)
    *(.got.plt)
    . = ALIGN(0x1000);
    _kernelDataEnd = ABSOLUTE(. - SHIFT);
  }

  .bss ALIGN(0x1000) :
    AT(ADDR(.bss) - SHIFT)
  {
    *(.bss)
    . = ALIGN(0x1000);
    _kernelBssEnd = ABSOLUTE(. - SHIFT);
  }

  . = VIRTUAL_HEAP;
  _heapBase = .;

  /DISCARD/ :
  {
    *(.comment)
    *(.eh_frame)
  }
}
