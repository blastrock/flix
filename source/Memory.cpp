#include "Memory.hpp"
#include "Util.hpp"
#include "Debug.hpp"

XLL_LOG_CATEGORY("core/memory/memorymap");

Memory& Memory::get()
{
  static Memory memory;
  return memory;
}

page_t Memory::getFreePage()
{
  xDeb("Free page request");

  for (page_t i = 0; i < _frames.size(); ++i)
    if (!_frames[i])
    {
      xDeb("Free page got");
      _frames[i] = true;
      return i;
    }
  xDeb("No more freepage, enlarging...");
  page_t i = _frames.size();
  _frames.resize(i+16, false);
  _frames[i] = true;
  return i;
}

void Memory::setPageFree(page_t page)
{
  //if (_frames.size() <= page)
  //  _frames.resize(intAlignSup(page+1, 16));

  assert(_frames.size() > page);
  assert(_frames[page]);

  _frames[page] = false;
}

void Memory::setPageUsed(page_t page)
{
  if (_frames.size() <= page)
    _frames.resize(intAlignSup(page+1, 16));

  assert(_frames.size() > page);
  assert(!_frames[page]);

  _frames[page] = true;
}

void Memory::setRangeUsed(page_t from, page_t to)
{
  assert(from <= to);

  for (; from < to; ++from)
    setPageUsed(from);
}

void Memory::completeRangeUsed(page_t from, page_t to)
{
  assert(from <= to);

  for (; from < to; ++from)
    if (from >= _frames.size() || !_frames[from])
      setPageUsed(from);
}
