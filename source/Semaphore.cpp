#include "Semaphore.hpp"
#include "TaskManager.hpp"

XLL_LOG_CATEGORY("support/sync/semaphore");

struct Semaphore::Waiter
{
  explicit Waiter(Task& task)
    : task(task)
  {}

  bool up = false;
  Task& task;

  // we need an intrusive list to avoid allocations in this critical code path
  Waiter* next = nullptr;
};

void Semaphore::down()
{
  xDeb("%p: Semaphore down", this);

  assert(
      SpinLock::getLockCount() == 0 &&
      "no spinlock must be taken during a semaphore down (which might block)");

  auto _ = _spinlock.getScoped();

  if (_count)
  {
    xDeb("No contetion");
    --_count;
  }
  else
  {
    xDeb("There is contention");
    // FIXME what if the task finishes before the wait is finished?
    auto& tm = *TaskManager::get();
    Waiter waiter{ tm.getActiveTask() };

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

    do
    {
      tm.prepareMeForSleep();
      auto _ = _spinlock.getScopedUnlock();
      tm.putMeToSleep();
    } while (!waiter.up);
  }
}

void Semaphore::up()
{
  xDeb("%p: Semaphore up", this);

  auto _ = _spinlock.getScoped();

  if (!_firstWaiter)
  {
    xDeb("No waiter");
    assert(!_lastWaiter);

    ++_count;
  }
  else
  {
    xDeb("At least one waiter");
    assert(_lastWaiter);

    Waiter* waiter = _firstWaiter;
    if (_firstWaiter == _lastWaiter)
      _lastWaiter = nullptr;
    _firstWaiter = _firstWaiter->next;

    waiter->up = true;
    TaskManager::get()->wakeUpTask(waiter->task);
  }
}
