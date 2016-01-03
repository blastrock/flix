#ifndef COND_VAR_HPP
#define COND_VAR_HPP

#include "SpinLock.hpp"

class Mutex;

class CondVar
{
public:
  CondVar() = default;
  CondVar(const CondVar&) = delete;
  CondVar(CondVar&&) = delete;
  CondVar& operator=(const CondVar&) = delete;
  CondVar& operator=(CondVar&&) = delete;

  void wait(Mutex& mutex);

  void notify_one();
  void notify_all();

private:
  struct Waiter;

  SpinLock _lock;
  Waiter* _firstWaiter;
  Waiter* _lastWaiter;
  unsigned _waiterCount;
};

#endif
