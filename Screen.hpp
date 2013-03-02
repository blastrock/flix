#ifndef SCREEN_HPP
#define SCREEN_HPP

#include <cstdint>
#include "IntUtil.hpp"
#include <cstring>
#include "io.hpp"

struct Position
{
  int x;
  int y;
};

enum class Color
{
  Black,
  Blue,
  Green,
  Cyan,
  Red,
  Magenta,
  Brown,
  LightGrey,
  DarkGrey,
  LightBlue,
  LightGreen,
  LightCyan,
  LightRed,
  LightMagenta,
  LightBrown,
  White
};

class Screen
{
  public:
    inline static void clear();
    inline static void putString(const char* c, Color fg = Color::LightGrey, Color bg = Color::Black);
    inline static void putInt(int value, Color fg = Color::LightGrey, Color bg = Color::Black);
    inline static void putHex(unsigned int value, Color fg = Color::LightGrey, Color bg = Color::Black);
    inline static void putChar(char c, Color fg = Color::LightGrey, Color bg = Color::Black);
    inline static void putChar(Position pos, char c, Color fg = Color::LightGrey, Color bg = Color::Black);
    inline static void updateCursor();
    inline static void scrollOneLine();

  private:
    static Position g_cursor;
};

void Screen::clear()
{
  std::memset((void*)0xB8000, 0, sizeof(uint16_t)*80*25);
  g_cursor = {0, 0};
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

void Screen::putInt(int value, Color fg, Color bg)
{
  putString(IntUtil::intToStr(value, 10), fg, bg);
}

void Screen::putHex(unsigned int value, Color fg, Color bg)
{
  putString(IntUtil::uintToStr(value, 16), fg, bg);
}

void Screen::putChar(char c, Color fg, Color bg)
{
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
}

void Screen::putChar(Position pos, char c, Color fg, Color bg)
{
  uint16_t val = c | ((uint16_t)fg) << 8 | ((uint16_t)bg) << 12;
  uint16_t* screen = (uint16_t*)0xB8000;
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
  std::memcpy((void*)0xB8000, (void*)(0xB8000 + 80*sizeof(uint16_t)), sizeof(uint16_t)*80*24);
  std::memset((void*)(0xB8000 + 24*80*sizeof(uint16_t)), 0, 80*sizeof(uint16_t));
}

#endif /* SCREEN_HPP */
