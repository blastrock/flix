#include "Task.hpp"
#include "Cpu.hpp"
#include "Interrupt.hpp"
#include "Debug.hpp"
#include "Symbols.hpp"

TaskManager* TaskManager::instance;

[[noreturn]] extern "C" void jump(Task::Context* task);

TaskManager* TaskManager::get()
{
  if (!instance)
    instance = new TaskManager;
  return instance;
}

TaskManager::TaskManager()
  : _currentTask(0)
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
}

void TaskManager::addTask(Task&& t)
{
  _tasks.push_back(std::move(t));
}

void TaskManager::terminateCurrentTask()
{
  Degf("Terminating task %d, size %d", _currentTask, _tasks.size());
  assert(_currentTask < _tasks.size());
  _tasks.erase(_tasks.begin() + _currentTask);
}

Task TaskManager::newKernelTask()
{
  Task task{};
  // see DescTables.cpp
  task.context.cs = 0x08;
  task.context.ss = 0x10;
  task.context.rflags = 0x0200; // enable IRQ
  task.pageDirectory.mapKernel();
  return task;
}

Task TaskManager::newUserTask()
{
  Task task{};
  // see DescTables.cpp
  task.context.cs = 0x1B;
  task.context.ss = 0x23;
  task.context.rflags = 0x0200; // enable IRQ
  task.pageDirectory.mapKernel();
  return task;
}

void TaskManager::saveCurrentTask(const Task::Context& ctx)
{
  Degf("Saving task %d with rip %x and rsp %x", _currentTask, ctx.rip, ctx.rsp);
  assert(_currentTask < _tasks.size());
  _tasks[_currentTask].context = ctx;
}

void TaskManager::scheduleNext()
{
  if (_tasks.empty())
    PANIC("Nothing to schedule!");

  ++_currentTask;
  if (_currentTask >= _tasks.size())
    _currentTask = 0;

  Task& nextTask = _tasks[_currentTask];
  assert(nextTask.context.rflags & (1 << 9) && "Interrupts were disabled in a task");
  Degf("Restoring task %d with rip %x and rsp %x", _currentTask,
      nextTask.context.rip, nextTask.context.rsp);
  nextTask.pageDirectory.use();
  jump(&nextTask.context);
}
