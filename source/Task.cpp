#include "Task.hpp"
#include "Cpu.hpp"
#include "Interrupt.hpp"
#include "Debug.hpp"

TaskManager* TaskManager::instance;

extern "C" void jump(Task* task);

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
  auto* tss = reinterpret_cast<TaskStateSegment*>(0xffffffff90000000 - 0x4000);
  std::memset(tss, 0, sizeof(tss));
  const int SIZE = 0x1000 * 4;
  auto* kernelStack = new char[SIZE];
  tss->ist1 = kernelStack + SIZE - 0x40;
}

void TaskManager::addTask(const Task& t)
{
  _tasks.push_back(t);
}

Task TaskManager::newKernelTask()
{
  Task task{};
  task.cs = 0x08;
  task.ss = 0x10;
  task.rflags = 0x0200; // enable IRQ
  return task;
}

void TaskManager::saveCurrentTask(const Task& t)
{
  Degf("Saving task %d with rip %x and rsp %x", _currentTask, t.rip, t.rsp);
  _tasks[_currentTask] = t;
}

void TaskManager::scheduleNext()
{
  if (_tasks.empty())
    PANIC("Nothing to schedule!");

  ++_currentTask;
  if (_currentTask == _tasks.size())
    _currentTask = 0;

  Task& t = _tasks[_currentTask];
  t.rflags |= 1 << 9;
  Degf("Restoring task %d with rip %x and rsp %x", _currentTask, t.rip, t.rsp);
  jump(&t);
}
