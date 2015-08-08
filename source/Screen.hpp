#ifndef SCREEN_HPP
#define SCREEN_HPP

#include <stddef.h>

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
    static void clear();
    static void putString(const char* c,
        size_t size,
        Color fg = Color::LightGrey,
        Color bg = Color::Black);
    static void putString(
        const char* c, Color fg = Color::LightGrey, Color bg = Color::Black);
    static void putChar(
        char c, Color fg = Color::LightGrey, Color bg = Color::Black);
    static void putChar(Position pos,
        char c,
        Color fg = Color::LightGrey,
        Color bg = Color::Black);
    static void updateCursor();

  private:
    static Position g_cursor;

    static void scrollOneLine();
};

#endif /* SCREEN_HPP */
