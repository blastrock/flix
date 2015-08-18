#include "Tty.hpp"
#include "Screen.hpp"
#include "Util.hpp"

Tty* Tty::getCurrent()
{
  static Tty* tty = new Tty();
  return tty;
}

void Tty::feedInput(char data)
{
  _stdin.push(data);
  Screen::putChar(data);
}

void Tty::print(const char* buf, size_t size)
{
  Screen::putString(buf, size);
}

size_t Tty::readInto(char* buf, size_t size)
{
  DisableInterrupts _;

  size_t toRead = std::min(size, _stdin.size());
  for (char* ptr = buf, * end = buf + toRead;
      ptr != end;
      ++ptr)
  {
    *ptr = _stdin.front();
    _stdin.pop();
  }
  return toRead;
}
