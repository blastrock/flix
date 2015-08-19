#include "TaskManager.hpp"
#include "Cpu.hpp"
#include "Interrupt.hpp"
#include "Debug.hpp"
#include "Symbols.hpp"
#include "DescTables.hpp"
#include "Util.hpp"

TaskManager* TaskManager::instance;

[[noreturn]] extern "C" void jump(Task::Context* task);

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
}

void TaskManager::setUpTss()
{
  // allocate a stack for interrupt handling
  const int SIZE = 0x1000 * 4;
  auto* kernelStack = new char[SIZE];

  // initialize the TSS (which is at the bottom of the current stack)
  auto* tss = reinterpret_cast<TaskStateSegment*>(
      reinterpret_cast<uintptr_t>(Symbols::getStackBase()) - 0x4000);
  std::memset(tss, 0, sizeof(*tss));
  // make it point to the top of the stack
  tss->ist1 = kernelStack + SIZE;
  Cpu::setKernelStack(kernelStack + SIZE);
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
  Degf("Adding new task with tid %d", t.tid);
  _tasks.insert(std::move(t));
}

void TaskManager::terminateCurrentTask()
{
  DisableInterrupts _;

  Degf("Terminating task %d, size %d", _activeTask, _tasks.size());
  const auto iter = _tasks.find(_activeTask);
  assert(iter != _tasks.end());
  _tasks.erase(iter);
  // TODO free stack
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

  Degf("Saving task %d with rip %x and rsp %x", _activeTask, ctx.rip, ctx.rsp);
  getActiveTask().context = ctx;
}

Task& TaskManager::getActiveTask()
{
  const auto iter = _tasks.find(_activeTask);
  assert(iter != _tasks.end());
  return const_cast<Task&>(*iter);
}

void TaskManager::scheduleNext()
{
  DisableInterrupts _;

  if (_tasks.empty())
    PANIC("Nothing to schedule!");

  auto iter = _tasks.upper_bound(_activeTask);
  if (iter == _tasks.end())
    iter = _tasks.begin();
  Task& nextTask = const_cast<Task&>(*iter);
  _activeTask = nextTask.tid;
  assert((nextTask.context.rflags & (1 << 9)) && "Interrupts were disabled in a task");
  Degf("Restoring task %d with rip %x and rsp %x", _activeTask,
      nextTask.context.rip, nextTask.context.rsp);
  nextTask.pageDirectory.use();
  jump(&nextTask.context);
}

void TaskManager::rescheduleSelf()
{
  assert(!_tasks.empty());

  Task& nextTask = getActiveTask();
  assert((nextTask.context.rflags & (1 << 9)) && "Interrupts were disabled in a task");
  Degf("Rescheduling self at %x", nextTask.context.rip);
  jump(&nextTask.context);
}
