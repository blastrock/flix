#include "CondVar.hpp"
#include "Mutex.hpp"
#include "TaskManager.hpp"

struct CondVar::Waiter
{
  Waiter(Task& task)
    : task(task)
  {}

  bool up = false;
  Task& task;
};

void CondVar::wait(Mutex& waitMutex)
{
  Degf("Waiting on condvar");

  // FIXME what if the task finishes before the wait is finished?
  auto& tm = *TaskManager::get();
  Waiter waiter{ tm.getActiveTask() };

  auto _ = _lock.getScoped();

  // if we had used RAII unlock of waitMutex here, we would have tried to
  // relock it even on spurious wake-ups. this way, we try to lock it only when
  // we are sure the condition has triggered
  do
  {
    _waiters.push_back(&waiter);

    {
      Degf("Going to sleep");

      tm.prepareMeForSleep();
      waitMutex.unlock();
      auto _ = _lock.getScopedUnlock();
      tm.putMeToSleep();
      Degf("Woke up");
    }
  } while (!waiter.up);

  Degf("Cond triggered");

  waitMutex.lock();

  Degf("Mutex relocked");
}

void CondVar::notify_one()
{
  Degf("Notifying one");

  Waiter* curWaiter;
  {
    auto _ = _lock.getScoped();
    curWaiter = _waiters.back();
    _waiters.pop_back();
  }

  curWaiter->up = true;
  TaskManager::get()->wakeUpTask(curWaiter->task);
}

void CondVar::notify_all()
{
  Degf("Notifying all");

  std::vector<Waiter*> curWaiters;
  // we are going to wake up a lot of threads, release the lock as soon as
  // possible
  {
    auto _ = _lock.getScoped();
    std::swap(curWaiters, _waiters);
  }

  for (auto* waiter : curWaiters)
  {
    waiter->up = true;
    TaskManager::get()->wakeUpTask(waiter->task);
  }
}
