#include "Tty.hpp"
#include "Screen.hpp"
#include "Util.hpp"
#include "Debug.hpp"

Tty* Tty::getCurrent()
{
  static Tty* tty = new Tty();
  return tty;
}

void Tty::feedInput(char data)
{
  Degf("Got input");

  auto _ = _mutex.getScoped();

  _stdin.push(data);
  Screen::putChar(data);

  if (data == '\n')
    _condvar.notify_all();

  Degf("Notified");
}

void Tty::print(const char* buf, size_t size)
{
  Screen::putString(buf, size);
}

size_t Tty::readInto(char* buf, size_t size)
{
  if (size == 0)
    return 0;

  Degf("Reading %d bytes from TTY", size);

  auto _ = _mutex.getScoped();

  while (_stdin.empty())
  {
    Degf("TTY empty, waiting");
    _condvar.wait(_mutex);
  }

  Degf("Starting read");

  size_t toRead = std::min(size, _stdin.size());
  for (char* ptr = buf, * end = buf + toRead;
      ptr != end;
      ++ptr)
  {
    *ptr = _stdin.front();
    _stdin.pop();
  }
  Degf("Read %d bytes", toRead);
  return toRead;
}
