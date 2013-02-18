#include "Memory.hpp"
#include "KHeap.hpp"
#include "Debug.hpp"

BitVector Memory::g_frames;

void Memory::init(Multiboot const& mboot)
{
  if (!(mboot.flags & (1 << 6)))
  {
    debug("no memory info", 0);
    return;
  }

  u64 maxAddr = 0;
  forEachRange(mboot, [&maxAddr](u64 baseAddr, u64 length){
      //debug("base_addr", baseAddr);
      //debug("length", length);
      u64 end = baseAddr + length;
      if (end > maxAddr)
        maxAddr = end;
      });

  u8* frames = (u8*)KHeap::kmalloc_a(maxAddr/8);
  g_frames.setData(maxAddr/8, frames);
  g_frames.fill(false);

  // TODO optimize lawl...
  // XXX constants
  forEachRange(mboot, [](u64 baseAddr, u64 length){
      for (u64 page = (baseAddr + 0x1000 - 1) / 0x1000,
          end = (baseAddr + length) / 0x1000;
          page < end;
          ++page)
        g_frames.setBit(page, true);
      });

  // XXX constants
  for (u64 page = 0x100000 / 0x1000,
      end = reinterpret_cast<u64>(KHeap::kmalloc_a(0)) / 0x1000;
      page < end;
      ++page)
    g_frames.setBit(page, false);
}

template <typename T>
void Memory::forEachRange(Multiboot const& mboot, T func)
{
  MapRange const* range = reinterpret_cast<MapRange const*>(mboot.mmap_addr);
  MapRange const* const endRange = reinterpret_cast<MapRange const*>(
    reinterpret_cast<char const*>(range) + mboot.mmap_length);
  for (;
      range < endRange;
      range = reinterpret_cast<MapRange const*>(
        reinterpret_cast<char const*>(range) + range->size + 4))
    if (range->type == 1)
      func(range->base_addr, range->length);
  //{
  //  debug("size", range->size);
  //  debug("base", range->base_addr);
  //  debug("len", range->length);
  //  debug("type", range->type);
  //  debug("", 0);
  //}
}
