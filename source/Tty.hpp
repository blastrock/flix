#ifndef TTY_HPP
#define TTY_HPP

#include <stddef.h>
#include <deque>

#include "Mutex.hpp"
#include "CondVar.hpp"

class Tty
{
public:
  static Tty* getCurrent();

  void feedInput(char data);

  void print(const char* buf, size_t size);
  size_t readInto(char* buf, size_t size);

private:
  std::deque<char> _stdin;
  Mutex _mutex;
  CondVar _condvar;
};

#endif
