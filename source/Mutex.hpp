#ifndef MUTEX_HPP
#define MUTEX_HPP

#include "Semaphore.hpp"
#include "Util.hpp"

class Mutex
{
public:
  using Scoped = ScopedLock<Mutex>;

  Mutex()
    : _semaphore(1)
  {}

  Mutex(const Mutex&) = delete;
  Mutex(Mutex&&) = delete;
  Mutex& operator=(const Mutex&) = delete;
  Mutex& operator=(Mutex&&) = delete;

  Scoped getScoped()
  {
    return Scoped(*this);
  }

  void lock()
  {
    _semaphore.down();
  }
  void unlock()
  {
    _semaphore.up();
  }

private:
  Semaphore _semaphore;
};

#endif
