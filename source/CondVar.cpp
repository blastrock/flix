#include "CondVar.hpp"
#include "Mutex.hpp"
#include "TaskManager.hpp"

XLL_LOG_CATEGORY("support/sync/condvar");

struct CondVar::Waiter
{
  explicit Waiter(Task& task)
    : task(task)
  {}

  bool up = false;
  Task& task;

  // we need an intrusive list to avoid allocations in this critical code path
  Waiter* next = nullptr;
};

void CondVar::wait(Mutex& waitMutex)
{
  xDeb("Waiting on condvar");

  // FIXME what if the task finishes before the wait is finished?
  auto& tm = *TaskManager::get();
  Waiter waiter{ tm.getActiveTask() };

  {
    auto _ = _lock.getScoped();

    ++_waiterCount;

    if (_lastWaiter)
    {
      _lastWaiter->next = &waiter;
      _lastWaiter = &waiter;
    }
    else
    {
      assert(!_firstWaiter);
      _firstWaiter = _lastWaiter = &waiter;
    }

    // if we had used RAII unlock of waitMutex here, we would have tried to
    // relock it even on spurious wake-ups. this way, we try to lock it only when
    // we are sure the condition has triggered
    do
    {
      xDeb("Going to sleep");
      tm.prepareMeForSleep();
      waitMutex.unlock();
      auto _ = _lock.getScopedUnlock();
      tm.putMeToSleep();
      xDeb("Woke up");
    } while (!waiter.up);

    xDeb("Cond triggered");
  }

  waitMutex.lock();

  xDeb("Mutex relocked");
}

void CondVar::notify_one()
{
  xDeb("Notifying one");

  Waiter* curWaiter;
  {
    auto _ = _lock.getScoped();
    if (!_firstWaiter)
    {
      xDeb("No waiter");
      assert(!_lastWaiter);
      return;
    }

    curWaiter = _firstWaiter;
    if (_firstWaiter == _lastWaiter)
      _lastWaiter = nullptr;
    _firstWaiter = _firstWaiter->next;
    --_waiterCount;
  }

  curWaiter->up = true;
  TaskManager::get()->wakeUpTask(curWaiter->task);
}

void CondVar::notify_all()
{
  xDeb("Notifying all");

  std::vector<Waiter*> curWaiters;
  // allocate before we take the lock
  curWaiters.reserve(_waiterCount);
  // we are going to wake up a lot of threads, release the lock as soon as
  // possible
  {
    auto _ = _lock.getScoped();
    Waiter* curWaiter = _firstWaiter;
    unsigned count = 0;
    while (curWaiter)
    {
      ++count;
      curWaiters.push_back(curWaiter);
      curWaiter = curWaiter->next;
    }
    _firstWaiter = _lastWaiter = nullptr;
    _waiterCount = 0;
  }

  for (auto* waiter : curWaiters)
  {
    waiter->up = true;
    TaskManager::get()->wakeUpTask(waiter->task);
  }
}
