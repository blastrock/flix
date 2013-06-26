#include "Memory.hpp"
#include "Util.hpp"
#include "Debug.hpp"

std::vector<bool> Memory::g_frames;

uint64_t Memory::getFreePage()
{
  fDeg() << "free page";

  for (uint64_t i = 0; i < g_frames.size(); ++i)
    if (!g_frames[i])
    {
      g_frames[i] = true;
      return i;
    }
  uint64_t i = g_frames.size();
  g_frames.resize(i+16, false);
  g_frames[i] = true;
  return i;
}

void Memory::setPageFree(uint64_t page)
{
  //if (g_frames.size() <= page)
  //  g_frames.resize(intAlignSup(page+1, 16));

  assert(g_frames.size() > page);
  assert(g_frames[page]);

  g_frames[page] = false;
}

void Memory::setPageUsed(uint64_t page)
{
  if (g_frames.size() <= page)
    g_frames.resize(intAlignSup(page+1, 16));

  assert(g_frames.size() > page);

  g_frames[page] = true;
}
