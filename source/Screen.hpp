#ifndef SCREEN_HPP
#define SCREEN_HPP

#include <stddef.h>
#include "SpinLock.hpp"

class Screen
{
public:
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
    White,
  };

  static Screen& getInstance();

  void clear();
  void putString(const char* c,
      size_t size,
      Color fg = Color::LightGrey,
      Color bg = Color::Black);
  void putString(
      const char* c, Color fg = Color::LightGrey, Color bg = Color::Black);
  void putChar(char c, Color fg = Color::LightGrey, Color bg = Color::Black);
  void putChar(Position pos,
      char c,
      Color fg = Color::LightGrey,
      Color bg = Color::Black);
  void updateCursor();

private:
  SpinLock _lock;
  Position _cursor = {};

  void scrollOneLine();
};

#endif /* SCREEN_HPP */
