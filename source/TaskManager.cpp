#include "TaskManager.hpp"
#include "Cpu.hpp"
#include "Interrupt.hpp"
#include "Debug.hpp"
#include "Symbols.hpp"
#include "DescTables.hpp"
#include "Util.hpp"

XLL_LOG_CATEGORY("core/taskmanager");

TaskManager* TaskManager::instance;

[[noreturn]] extern "C" void jump(Task::Context* task);
extern "C" int task_save(Task::Context* task);

TaskManager* TaskManager::get()
{
  if (!instance)
    instance = new TaskManager;
  return instance;
}

TaskManager::TaskManager()
  : _activeTask(0)
  , _nextTid(1)
{
  // for the moment there are 0 task, so we don't need to tick
  Interrupt::mask(IRQ_TIMER);
}

void TaskManager::setUpTss()
{
  _tss = new TaskStateSegment{};
  std::memset(_tss, 0, sizeof(*_tss));
  DescTables::initTr(_tss);

  setKernelStack();
}

void TaskManager::updateNextTid()
{
  // TODO handle the case where the set is full
  while (_tasks.find(_nextTid) != _tasks.end())
    ++_nextTid;
}

void TaskManager::addTask(Task&& t)
{
  DisableInterrupts _;

  updateNextTid();

  t.tid = _nextTid;
  ++_nextTid;
  xDeb("Adding new task with tid %d", t.tid);
  _tasks.insert(std::move(t));

  doInterruptMasking();
}

void TaskManager::terminateCurrentTask()
{
  DisableInterrupts _;

  xDeb("Terminating task %d, size %d", _activeTask, _tasks.size());
  const auto iter = _tasks.find(_activeTask);
  assert(iter != _tasks.end());
  _tasks.erase(iter);
  // TODO free stack

  doInterruptMasking();
}

Task TaskManager::newKernelTask()
{
  Task task{};
  task.context.cs = DescTables::SYSTEM_CS;
  task.context.ss = DescTables::SYSTEM_DS;
  task.context.rflags = 0x0200; // enable IRQ
  task.pageDirectory.mapKernel();
  return task;
}

Task TaskManager::newUserTask()
{
  Task task{};
  task.context.cs = DescTables::USER_CS | 0x3;
  task.context.ss = DescTables::USER_DS | 0x3;
  task.context.rflags = 0x0200; // enable IRQ
  task.pageDirectory.mapKernel();
  return task;
}

void TaskManager::downgradeCurrentTask()
{
  Task& task = getActiveTask();
  task.context.cs = DescTables::USER_CS | 0x3;
  task.context.ss = DescTables::USER_DS | 0x3;
}

void TaskManager::saveCurrentTask(const Task::Context& ctx)
{
  DisableInterrupts _;

  xDeb("Saving task %d with rip %x and rsp %x", _activeTask, ctx.rip, ctx.rsp);
  getActiveTask().context = ctx;

  assert((ctx.cs == DescTables::SYSTEM_CS ||
        (ctx.rflags & (1 << 9))) &&
      "Interrupts were disabled in a user task");
}

void TaskManager::prepareMeForSleep(bool interrupts)
{
  Task& task = getActiveTask();
  task.state = Task::State::Sleeping;
  task.context.rflags |= interrupts ? 1 << 9 : 0;

  doInterruptMasking();
}

void TaskManager::putMeToSleep()
{
  xDeb("Putting task to sleep");

  // interrupts may already be disabled if we come from a syscall/interrupt
  if (Cpu::rflags() & (1 << 9))
    disableInterrupts();

  Task& task = getActiveTask();
  xDeb("Task rip:%x rsp:%x", task.context.rip, task.context.rsp);

  if (task.state != Task::State::Sleeping)
  {
    xDeb("Task has been woken up before going to sleep");
    return;
  }

  const bool intactive = task.context.rflags & (1 << 9);

  if (!task_save(&task.context))
  {
    assert(task.context.cs == DescTables::SYSTEM_CS &&
        "putMeToSleep called from userspace");

    if (intactive)
      task.context.rflags |= (1 << 9);

    scheduleNext(); // going to sleep
  }
  else
    return; // waking up
}

void TaskManager::wakeUpTask(Task& task)
{
  task.state = Task::State::Runnable;

  doInterruptMasking();
}

bool TaskManager::isTaskActive()
{
  return _activeTask;
}

Task* TaskManager::getActive()
{
  if (_activeTask == 0)
    return nullptr;
  return &getActiveTask();
}

Task& TaskManager::getActiveTask()
{
  const auto iter = _tasks.find(_activeTask);
  assert(iter != _tasks.end());
  return const_cast<Task&>(*iter);
}

TaskManager::Tasks::iterator TaskManager::getNext()
{
  auto id = _activeTask;
  bool looped = false;
  while (true)
  {
    auto iter = _tasks.upper_bound(id);
    if (iter == _tasks.end())
    {
      if (looped)
        return _tasks.end();

      id = 0;
      looped = true;
      continue;
    }
    if (iter->state == Task::State::Runnable)
      return iter;
    ++id;
  }
}

void TaskManager::setKernelStack()
{
  auto kernelStack = Symbols::getStackBase();

  _tss->ist1 = kernelStack;
  Cpu::setKernelStack(kernelStack);
}

void TaskManager::enterSleep()
{
  _activeTask = 0;

  setKernelStack();

  enableInterrupts();
  asm volatile("hlt":::"memory");

  PANIC("returned from hlt in enterSleep");
}

void TaskManager::tryScheduleNext()
{
  if (_tasks.empty())
    PANIC("Nothing to schedule!");

  const auto iter = getNext();
  if (iter == _tasks.end())
    return;

  Task& nextTask = const_cast<Task&>(*iter);
  _activeTask = nextTask.tid;
  assert((nextTask.context.cs == DescTables::SYSTEM_CS ||
        (nextTask.context.rflags & (1 << 9))) &&
      "Interrupts were disabled in a user task");
  xDeb("Restoring task %d with rip %x and rsp %x (int: %s)", _activeTask,
      nextTask.context.rip, nextTask.context.rsp,
      !!(nextTask.context.rflags & (1 << 9)));

  // set the page directory of the process
  nextTask.pageDirectory.use();

  // set the kernel stack for interrupt/syscall handling
  _tss->ist1 = nextTask.kernelStackTop;
  Cpu::setKernelStack(nextTask.kernelStackTop);

  jump(&nextTask.context);
}

void TaskManager::scheduleNext()
{
  tryScheduleNext();
  xDeb("All processes sleeping, entering kernel sleep");
  enterSleep();
}

void TaskManager::rescheduleSelf()
{
  assert(!_tasks.empty());

  Task& nextTask = getActiveTask();
  assert((nextTask.context.cs == DescTables::SYSTEM_CS ||
        (nextTask.context.rflags & (1 << 9))) &&
      "Interrupts were disabled in a user task");
  xDeb("Rescheduling self at %x", nextTask.context.rip);
  jump(&nextTask.context);
}

void TaskManager::doInterruptMasking()
{
  DisableInterrupts _;

  bool foundFirst = false;

  for (const auto& task : _tasks)
    if (task.state == Task::Runnable)
    {
      if (foundFirst)
      {
        xDeb("Ticking disabled");
        Interrupt::unmask(IRQ_TIMER);
        return;
      }
      else
        foundFirst = true;
    }

  xDeb("Ticking enabled");
  Interrupt::mask(IRQ_TIMER);
}
