#ifndef SCREEN_HPP
#define SCREEN_HPP

#include "inttypes.hpp"
#include "string.h"

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
    inline static void putString(const char* c, Color fg = Color::LightGrey, Color bg = Color::Black);
    inline static void putChar(char c, Color fg = Color::LightGrey, Color bg = Color::Black);
    inline static void putChar(Position pos, char c, Color fg = Color::LightGrey, Color bg = Color::Black);
    inline static void clear();

  private:
    static Position g_cursor;
};

void Screen::putString(const char* c, Color fg, Color bg)
{
  while (*c)
  {
    putChar(*c, fg, bg);

    ++c;
  }
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
}

void Screen::putChar(Position pos, char c, Color fg, Color bg)
{
  u16 val = c | ((u16)fg) << 8 | ((u16)bg) << 12;
  u16* screen = (u16*)0xB8000;
  screen[pos.y * 80 + pos.x] = val;
}

void Screen::clear()
{
  memset((void*)0xB8000, 0, sizeof(u16)*80*25);
}

#endif /* SCREEN_HPP */
