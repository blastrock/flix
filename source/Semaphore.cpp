#include "Semaphore.hpp"
#include "TaskManager.hpp"

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
  Degf("Semaphore down");

  auto _ = _spinlock.getScoped();

  if (_count)
  {
    Degf("No contetion");
    --_count;
  }
  else
  {
    Degf("There is contention");
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
  Degf("Semaphore up");

  auto _ = _spinlock.getScoped();

  if (_waiters.empty())
  {
    Degf("No waiters");
    ++_count;
  }
  else
  {
    Degf("There are waiters");
    Waiter* waiter = _waiters.front();
    _waiters.pop();
    waiter->up = true;
    TaskManager::get()->wakeUpTask(waiter->task);
  }
}
