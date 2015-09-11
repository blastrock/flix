#include "Tty.hpp"
#include "Screen.hpp"
#include "Util.hpp"
#include "Debug.hpp"

XLL_LOG_CATEGORY("core/tty");

Tty* Tty::getCurrent()
{
  static Tty* tty = new Tty();
  return tty;
}

void Tty::feedInput(char data)
{
  xDeb("Got input");

  auto _ = _mutex.getScoped();

  if (data == '\b')
  {
    if (!_stdin.empty())
    {
      _stdin.pop_back();
      Screen::putChar(data);
    }
    return;
  }

  _stdin.push_back(data);
  Screen::putChar(data);

  if (data == '\n')
    _condvar.notify_all();

  xDeb("Notified");
}

void Tty::print(const char* buf, size_t size)
{
  Screen::putString(buf, size);
}

size_t Tty::readInto(char* buf, size_t size)
{
  if (size == 0)
    return 0;

  xDeb("Reading %d bytes from TTY", size);

  auto _ = _mutex.getScoped();

  while (_stdin.empty())
  {
    xDeb("TTY empty, waiting");
    _condvar.wait(_mutex);
  }

  xDeb("Starting read");

  size_t toRead = std::min(size, _stdin.size());
  for (char* ptr = buf, * end = buf + toRead;
      ptr != end;
      ++ptr)
  {
    *ptr = _stdin.front();
    _stdin.pop_front();
  }
  xDeb("Read %d bytes", toRead);
  return toRead;
}
