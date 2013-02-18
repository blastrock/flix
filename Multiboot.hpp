#ifndef MULTIBOOT_HPP
#define MULTIBOOT_HPP

#include "cstdint"

struct Multiboot
{
  uint32_t flags;
  uint32_t mem_lower;
  uint32_t mem_upper;
  uint32_t boot_device;
  uint32_t cmdline;
  uint32_t mods_count;
  uint32_t mods_addr;
  uint8_t syms[16];
  uint32_t mmap_length;
  uint32_t mmap_addr;
} __attribute__((packed));

struct MapRange
{
  uint32_t size;
  uint64_t base_addr;
  uint64_t length;
  uint32_t type;
} __attribute__((packed));

#endif /* MULTIBOOT_HPP */
