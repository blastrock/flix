#include "Semaphore.hpp"
#include "TaskManager.hpp"

XLL_LOG_CATEGORY("support/sync/semaphore");

struct Semaphore::Waiter
{
  Waiter(Task& task)
    : task(task)
  {}

  bool up = false;
  Task& task;
};

void Semaphore::down()
{
  xDeb("%p: Semaphore down", this);

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
    do
    {
      _waiters.push(&waiter);

      {
        tm.prepareMeForSleep();
        auto _ = _spinlock.getScopedUnlock();
        tm.putMeToSleep();
      }
    } while (!waiter.up);
  }
}

void Semaphore::up()
{
  xDeb("%p: Semaphore up", this);

  auto _ = _spinlock.getScoped();

  xDeb("Number of waiters: %d", _waiters.size());
  if (_waiters.empty())
    ++_count;
  else
  {
    Waiter* waiter = _waiters.front();
    _waiters.pop();
    waiter->up = true;
    TaskManager::get()->wakeUpTask(waiter->task);
  }
}
