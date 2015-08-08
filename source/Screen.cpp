#include "Screen.hpp"
#include "Cpu.hpp"

#include <cstdint>
#include <cstring>
#include "io.hpp"

static constexpr uintptr_t VGA_BASE = 0xffffffffc1000000;

Position Screen::g_cursor = {0, 0};

void Screen::clear()
{
  std::memset((void*)VGA_BASE, 0, sizeof(uint16_t)*80*25);
  g_cursor = {0, 0};
}

void Screen::putString(const char* c, size_t size, Color fg, Color bg)
{
  bool i = Cpu::rflags() & (1 << 9);
  if (i)
    asm volatile ("cli");

  while (size--)
  {
    putChar(*c, fg, bg);
    ++c;
  }

  updateCursor();
  if (i)
    asm volatile ("sti");
}

void Screen::putString(const char* c, Color fg, Color bg)
{
  bool i = Cpu::rflags() & (1 << 9);
  if (i)
    asm volatile ("cli");

  while (*c)
  {
    putChar(*c, fg, bg);
    ++c;
  }

  updateCursor();
  if (i)
    asm volatile ("sti");
}

void Screen::putChar(char c, Color fg, Color bg)
{
  bool i = Cpu::rflags() & (1 << 9);
  if (i)
    asm volatile ("cli");
  switch (c)
  {
    case '\n':
      g_cursor.x = 0;
      ++g_cursor.y;
      break;
    case '\r':
      g_cursor.x = 0;
      break;
    default:
      putChar(g_cursor, c, fg, bg);

      ++g_cursor.x;
      if (g_cursor.x == 80)
      {
        g_cursor.x = 0;
        ++g_cursor.y;
      }
  }

  if (g_cursor.y == 25)
  {
    scrollOneLine();
    --g_cursor.y;
  }
  if (i)
    asm volatile ("sti");
}

void Screen::putChar(Position pos, char c, Color fg, Color bg)
{
  uint16_t val = c | ((uint16_t)fg) << 8 | ((uint16_t)bg) << 12;
  uint16_t* screen = (uint16_t*)VGA_BASE;
  screen[pos.y * 80 + pos.x] = val;
}

void Screen::updateCursor()
{
  uint16_t cursorLocation = g_cursor.y * 80 + g_cursor.x;
  io::outb(0x3D4, 14);                  // Tell the VGA board we are setting the high cursor byte.
  io::outb(0x3D5, cursorLocation >> 8); // Send the high cursor byte.
  io::outb(0x3D4, 15);                  // Tell the VGA board we are setting the low cursor byte.
  io::outb(0x3D5, cursorLocation);      // Send the low cursor byte.
}

void Screen::scrollOneLine()
{
  std::memcpy((void*)VGA_BASE, (void*)(VGA_BASE + 80*sizeof(uint16_t)), sizeof(uint16_t)*80*24);
  std::memset((void*)(VGA_BASE + 24*80*sizeof(uint16_t)), 0, 80*sizeof(uint16_t));
}
