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
  auto* tss = reinterpret_cast<TaskStateSegment*>(
      reinterpret_cast<uintptr_t>(Symbols::getStackBase()) - 0x4000);
  std::memset(tss, 0, sizeof(tss));
  const int SIZE = 0x1000 * 4;
  auto* kernelStack = new char[SIZE];
  tss->ist1 = kernelStack + SIZE - 0x40;
}

void TaskManager::addTask(Task&& t)
{
  _tasks.push_back(std::move(t));
}

void TaskManager::terminateCurrentTask()
{
  _tasks.erase(_tasks.begin() + _currentTask);
}

Task TaskManager::newKernelTask()
{
  Task task{};
  task.context.cs = 0x08;
  task.context.ss = 0x10;
  task.context.rflags = 0x0200; // enable IRQ
  task.pd.mapKernel();
  return task;
}

Task TaskManager::newUserTask()
{
  Task task{};
  task.context.cs = 0x1B;
  task.context.ss = 0x23;
  task.context.rflags = 0x0200; // enable IRQ
  task.pd.mapKernel();
  return task;
}

void TaskManager::saveCurrentTask(const Task::Context& ctx)
{
  Degf("Saving task %d with rip %x and rsp %x", _currentTask, ctx.rip, ctx.rsp);
  _tasks[_currentTask].context = ctx;
}

void TaskManager::scheduleNext()
{
  if (_tasks.empty())
    PANIC("Nothing to schedule!");

  ++_currentTask;
  if (_currentTask >= _tasks.size())
    _currentTask = 0;

  Task& t = _tasks[_currentTask];
  assert(t.context.rflags & (1 << 9));
  Degf("Restoring task %d with rip %x and rsp %x", _currentTask, t.context.rip, t.context.rsp);
  t.pd.use();
  jump(&t.context);
}
