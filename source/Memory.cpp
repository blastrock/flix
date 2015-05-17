#include "Memory.hpp"
#include "Util.hpp"
#include "Debug.hpp"

std::vector<bool> Memory::g_frames;

uintptr_t Memory::getFreePage()
{
  Degf("Free page request");

  for (uintptr_t i = 0; i < g_frames.size(); ++i)
    if (!g_frames[i])
    {
      Degf("Free page got");
      g_frames[i] = true;
      return i;
    }
  Degf("No more freepage, enlaring...");
  uintptr_t i = g_frames.size();
  g_frames.resize(i+16, false);
  g_frames[i] = true;
  return i;
}

void Memory::setPageFree(uintptr_t page)
{
  //if (g_frames.size() <= page)
  //  g_frames.resize(intAlignSup(page+1, 16));

  assert(g_frames.size() > page);
  assert(g_frames[page]);

  g_frames[page] = false;
}

void Memory::setPageUsed(uintptr_t page)
{
  if (g_frames.size() <= page)
    g_frames.resize(intAlignSup(page+1, 16));

  assert(g_frames.size() > page);

  g_frames[page] = true;
}
