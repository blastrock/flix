#include "Screen.hpp"
#include "Cpu.hpp"
#include "Util.hpp"
#include "Symbols.hpp"

#include <cstdint>
#include <cstring>
#include "io.hpp"

Screen& Screen::getInstance()
{
  static Screen g_screen;
  return g_screen;
}

void Screen::clear()
{
  auto _ = _lock.getScoped();

  std::memset(Symbols::getKernelVVga(), 0, sizeof(uint16_t)*80*25);
  _cursor = {0, 0};
}

void Screen::putString(const char* c, size_t size, Color fg, Color bg)
{
  while (size--)
  {
    putChar(*c, fg, bg);
    ++c;
  }

  updateCursor();
}

void Screen::putString(const char* c, Color fg, Color bg)
{
  while (*c)
  {
    putChar(*c, fg, bg);
    ++c;
  }

  updateCursor();
}

void Screen::putChar(char c, Color fg, Color bg)
{
  auto _ = _lock.getScoped();

  switch (c)
  {
    case '\n':
      _cursor.x = 0;
      ++_cursor.y;
      break;
    case '\r':
      _cursor.x = 0;
      break;
    case '\b':
      if (_cursor.x)
        --_cursor.x;
      putChar(_cursor, '\0', fg, bg);
      break;
    case '\t':
      _cursor.x += 8 - _cursor.x % 8;
      if (_cursor.x >= 80)
      {
        _cursor.x = 0;
        ++_cursor.y;
      }
      break;
    default:
      putChar(_cursor, c, fg, bg);

      ++_cursor.x;
      if (_cursor.x == 80)
      {
        _cursor.x = 0;
        ++_cursor.y;
      }
      break;
  }

  if (_cursor.y == 24)
  {
    scrollOneLine();
    --_cursor.y;
  }
  assert(_cursor.x < 80 && _cursor.x >= 0);
  assert(_cursor.y < 24 && _cursor.y >= 0);
}

void Screen::putChar(Position pos, char c, Color fg, Color bg)
{
  assert(pos.x < 80 && pos.x >= 0);
  assert(pos.y < 24 && pos.y >= 0);
  const uint16_t val = c | ((uint16_t)fg) << 8 | ((uint16_t)bg) << 12;
  auto const screen = reinterpret_cast<uint16_t*>(Symbols::getKernelVVga());
  screen[pos.y * 80 + pos.x] = val;
}

void Screen::updateCursor()
{
  auto _ = _lock.getScoped();

  uint16_t cursorLocation = _cursor.y * 80 + _cursor.x;
  // Tell the VGA board we are setting the high cursor byte.
  io::outb(0x3D4, 14);
  // Send the high cursor byte.
  io::outb(0x3D5, cursorLocation >> 8);
  // Tell the VGA board we are setting the low cursor byte.
  io::outb(0x3D4, 15);
  // Send the low cursor byte.
  io::outb(0x3D5, cursorLocation);
}

void Screen::scrollOneLine()
{
  std::memcpy(Symbols::getKernelVVga(),
      Symbols::getKernelVVga() + 80 * sizeof(uint16_t),
      sizeof(uint16_t) * 80 * 24);
  std::memset(Symbols::getKernelVVga() + 24 * 80 * sizeof(uint16_t),
      0,
      80 * sizeof(uint16_t));
}
