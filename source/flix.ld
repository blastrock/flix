VIRTUAL_BASE = 0xffffffff80000000;
PHYS_BASE = 0x100000;
HEAP_BASE = 0xffffffffc0000000;
SHIFT = VIRTUAL_BASE; /* - PHYS_BASE;*/

ENTRY(start)
SECTIONS
{
  . = PHYS_BASE;

  .bootstrap :
  {
    _kernelStart = ABSOLUTE(ADDR(.bootstrap));
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

  . = HEAP_BASE;
  _heapBase = .;

  /DISCARD/ :
  {
    *(.comment)
    *(.eh_frame)
  }
}
