#ifndef SEMAPHORE_HPP
#define SEMAPHORE_HPP

#include <queue>
#include <cstdint>

#include "SpinLock.hpp"

class Semaphore
{
public:
  using Scoped = ScopedLock<Semaphore>;

  Semaphore(uint32_t init)
    : _count(init)
  {}

  Semaphore(const Semaphore&) = delete;
  Semaphore(Semaphore&&) = delete;
  Semaphore& operator=(const Semaphore&) = delete;
  Semaphore& operator=(Semaphore&&) = delete;

  Scoped getScoped()
  {
    return Scoped(*this);
  }

  void down();
  void up();

  void lock()
  {
    down();
  }
  void unlock()
  {
    up();
  }

private:
  struct Waiter;

  uint32_t _count;
  std::queue<Waiter*> _waiters;
  SpinLock _spinlock;
};

#endif
