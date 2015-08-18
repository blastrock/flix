#ifndef TTY_HPP
#define TTY_HPP

#include <stddef.h>
#include <queue>

class Tty
{
public:
  static Tty* getCurrent();

  void feedInput(char data);

  void print(const char* buf, size_t size);
  size_t readInto(char* buf, size_t size);

private:
  std::queue<char> _stdin;
};

#endif
