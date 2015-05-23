Flix boot process
*****************

Grub (multiboot2)
=================

Bootstrapping
=============

Kernel main
===========

GDT initialization
------------------

The kernel GDT has 6 descriptors:

**A null descriptor**
  They say this is useful, somehow...

**A privileged code and data segment descriptor**
  These are mandatory. AMD's AMD64 doc says that only the P-bit (presence)
  matters, but not putting the W-bit (write) triggers a General Proection
  Fault.

**An unprivileged code and data segment descriptor**
  When running a user task, these segments must be used to tell the CPU that we
  are running on low privileges.

**A task segment**
  We need a task segment that's used when running user or kernel tasks. This
  allows interrupt control transfer to use a dedicated stack and not messing up
  the task's stack.

This table is hard-coded. At initialization, we:

- load the GDT through the ``lgdt`` instruction
- set all segment registers (ds, es, fs, gs, ss) to the privileged data segment
- do a long jump to the next instruction through the ``lretq`` instruction to
  change the cs register

When running a user task, we change only the cs and ss registers as I don't
know really how the other ones are used.

IDT initialization
------------------

First thing, the PIC is initialized and low IRQs are remapped to higher numbers
because they conflict with the CPU's exception mechanism. This may not be the
best place to put this code but it's okay.

Some CPU exceptions push an error-code on the stack that give more information
about what happened, but some do not. Our interruption handlers always push one
with a value of 0 if the exception is not one that pushes one. They are 16
exceptions + 32 PIC IRQs (which are split between a master and slave PIC, but
it's not important).

All the routines addresses are pushed into a global vector with a special
routine for interrupt ``0x80`` that will be our syscall, as it's done in Linux.

So we need to initialize our IDT with gates that point to these routines (and
empty gates for those without routine). We mark it as an interrupt gate that
switches to privilege level 0 and uses IST 1 to fetch a stack pointer to handle
the interrupt. All these gates are marked private (user-space can't trigger it
with the ``int`` instruction) except for the ``0x80`` which is the syscall
interrupt.

The table is then loaded with the ``lidt`` instruction.

At this point, no task is currently active and memory mapping is not ready (we
don't even have stack handle the interruption), so any interruption will cause
a triple fault.

Kernel heap initialization
--------------------------

The bootstrapping process has provided us with a heap of 2MB and has put the
Multiboot header at a strategic position so that it can be handled as a memory
block and freed later.

Out of the 2MB of memory that we have, we split it in two blocks. The first is
the Multiboot header (from which we read the size) and the second is a free
block that can be enlarged by allocating new pages as soon as pagination is up
and ready.

From now on, we can use dynamic allocation in the limit of 2MB - the size of
the multiboot header. That should be enough until pagination is ready.

Page heap initialization
------------------------

This is a different heap, more like a pool, to allocate pages needed for
pagination, like the page table and the layers above. It only sets its start
address. Bootstrap code has allocated 2MB for this heap which is enough to last
until the pagination is fully up.

Pagination initialization
-------------------------

We now recreate a full page directory using the premapped pages that we have
set up in bootstrap and initialized in the previous steps. This is the kernel
page directory, everything has the user-space bit set to 0.

We map the VGA IO so that we can print on the screen. Then we map the .text
section of the kernel as read-only and the .data and .bss sections as
read/write. We map the stack to the same place so that our code can keep
running. We map the page heap and the kernel heap that we just initialized so
that they continue working.

The bootstrap code is not remapped and can just be lost, we won't run it again.

All this is just remapping what is already mapped in the bootstrap page
directory, but this uses the ``PageManager`` structures which are more
extensible. This step can be seen as the ``PageManager`` initialization, but to
do that, we must remap everything.

After all this is set up, we switch to this new page directory by setting the
``cr3`` register.

Memory map initialization
-------------------------

After all this, we are still not ready to allocate new pages. We don't know yet
where to allocate those in physical memory. The multiboot loader should have
given us a map of the memory with sections where we can allocate and sections
reserved for various things like the BIOS.

We process this list and initialize our memory map so that it considers that
these sections are used and that we should not allocate stuff in them.

We also set pages used by our module (which is not part of the multiboot
header) as used so that they are not overwritten.

At this point, we are ready to allocate more that 2MB of memory. New page
allocation will work safely and won't overwrite critical stuff.

TSS initialization
------------------

We must now create a task so that interruptions switch to another stack. This
is needed even when there is no privilege level change because compiler can
generate code that stores things beyound the stack pointer, so stacking things
over it will overwrite data.

For the moment, the stack is allocated on the heap. We set it in IST1 and we
store the TSS at the bottom of the kernel stack (for the moment the address is
hardcoded, that's why we store it there).

Then we load the task descriptor through the ``ltr`` instruction. The
descriptor is already available in the GDT as described earlier.

Syscall vector initialization
-----------------------------

The syscall vector is not an x86-architecture concept, it's just a vector kept
in kernel space with the list of syscall entry points.

Syscall handlers are then registered in this vector.

Module loading
--------------

The multiboot loader provided us with a module. The specification allows to use
more than one module, but we need only one. This module is a CPIO archive that
will be used as the root filesystem.

First, the module is mapped into virtual memory just above the kernel stack.

Then, the archive is parsed and a VFS structure is created out of it. This
filesystem is set as the root filesystem.
