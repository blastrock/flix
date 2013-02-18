#ifndef MULTIBOOT_HPP
#define MULTIBOOT_HPP

#include "inttypes.hpp"

struct Multiboot
{
  u32 flags;
  u32 mem_lower;
  u32 mem_upper;
  u32 boot_device;
  u32 cmdline;
  u32 mods_count;
  u32 mods_addr;
  u8 syms[16];
  u32 mmap_length;
  u32 mmap_addr;
} __attribute__((packed));

struct MapRange
{
  u32 size;
  u64 base_addr;
  u64 length;
  u32 type;
} __attribute__((packed));

#endif /* MULTIBOOT_HPP */
