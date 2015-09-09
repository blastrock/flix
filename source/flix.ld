PHYSICAL_BASE = 0x200000;
BOOTSTRAP_SIZE = 0x10000;
VIRTUAL_BASE = 0xffffffffc0000000;
VIRTUAL_STACK = 0xffffffffd0000000;
VIRTUAL_PAGEHEAP = 0xffffffffe0000000;
VIRTUAL_HEAP = 0xfffffffff0000000;
SHIFT = VIRTUAL_BASE - PHYSICAL_BASE;

ENTRY(start)
SECTIONS
{
  . = PHYSICAL_BASE - BOOTSTRAP_SIZE;

  .bootstrap :
  {
    _kernelBootstrapStart = ABSOLUTE(.);
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

    /* force the section to be this large to keep alignment in file */
    /* this is not absolute, it's relative to the start of the section */
    . = BOOTSTRAP_SIZE;
  }

  . = VIRTUAL_BASE;
  _kernelTextVirt = .;

  .text ALIGN(0x1000) :
    AT(ADDR(.text) - SHIFT)
  {
    _kernelTextStart = ABSOLUTE(. - SHIFT);
    _kernelVTextStart = ABSOLUTE(.);
    *(.text*)
    . = ALIGN(0x8);
    _initArrayBegin = .;
    KEEP(*(.init_array))
    _initArrayEnd = .;
    . = ALIGN(0x1000);
    _kernelVTextEnd = ABSOLUTE(.);
  }

  .rodata ALIGN(0x1000) :
    AT(ADDR(.rodata) - SHIFT)
  {
    _kernelRodataStart = ABSOLUTE(. - SHIFT);
    _kernelVRodataStart = ABSOLUTE(.);
    *(.rodata*)
    . = ALIGN(0x1000);
    _kernelVRodataEnd = ABSOLUTE(.);
  }

  .data ALIGN(0x1000) :
    AT(ADDR(.data) - SHIFT)
  {
    _kernelDataStart = ABSOLUTE(. - SHIFT);
    _kernelVDataStart = ABSOLUTE(.);
    *(.data*)
    *(.got)
    *(.got.plt)
    . = ALIGN(0x1000);
    _kernelVDataEnd = ABSOLUTE(.);
  }

  .bss ALIGN(0x1000) :
    AT(ADDR(.bss) - SHIFT)
  {
    _kernelBssStart = ABSOLUTE(. - SHIFT);
    _kernelVBssStart = ABSOLUTE(.);
    *(.bss*)
    . = ALIGN(0x1000);
    _kernelBssEnd = ABSOLUTE(. - SHIFT);
    _kernelVBssEnd = ABSOLUTE(.);

    /* reserve space for the stuff below so that the boot loader does not use
     * this memory for something else */
    . = ALIGN(0x200000);
    . += 0x200000;
    _kernelPageHeapStart = ABSOLUTE(. - SHIFT);
    . += 0x200000;
    _kernelHeapStart = ABSOLUTE(. - SHIFT);
    . += 0x200000;
  }

  . = VIRTUAL_STACK;
  _stackBase = .;

  . = VIRTUAL_PAGEHEAP;
  _pageHeapBase = .;

  . = VIRTUAL_HEAP;
  _heapBase = .;

  /DISCARD/ :
  {
    *(.comment)
    *(.eh_frame)
    *(.gcc_except_table)
    /* *(.debug*) */
  }
}
