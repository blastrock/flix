#ifndef SCREEN_HPP
#define SCREEN_HPP

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

template <typename T>
class Singleton;

class Screen
{
  public:
    inline void putChar(Position pos, char c, Color fg = Color::LightGrey, Color bg = Color::Black);

  private:
    inline Screen();

    friend class Singleton<Screen>;
};

Screen::Screen()
{
}

void Screen::putChar(Position pos, char c, Color fg, Color bg)
{
  // FIXME
  short val = c | ((short)fg) << 8 | ((short)bg) << 12;
  short* screen = (short*)0xB8000;
  screen[pos.y * 80 + pos.x] = val;
}

#endif /* SCREEN_HPP */
