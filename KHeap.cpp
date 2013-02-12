// kheap.c -- Kernel heap functions, also provides
//            a placement malloc() for use before the heap is 
//            initialised.
//            Written for JamesM's kernel development tutorials.

#include "KHeap.hpp"

// end is defined in the linker script.
extern void* end;

char* KHeap::placement_address = (char*)&end;

void* KHeap::kmalloc(u32 sz, void** phys)
{
  if (phys)
    *phys = placement_address;
  char* tmp = placement_address;
  placement_address += sz;
  return tmp;
}

void* KHeap::kmalloc_a(u32 sz, void** phys)
{
  // This will eventually call malloc() on the kernel heap.
  // For now, though, we just assign memory at placement_address
  // and increment it by sz. Even when we've coded our kernel
  // heap, this will be useful for use before the heap is initialised.
  placement_address = (char*)(
      ((int)(placement_address + (0x1000 - 1))) & 0xFFFFF000);
  return kmalloc(sz, phys);
}
