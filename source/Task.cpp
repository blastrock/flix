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

void TaskManager::addTask(const Task& t)
{
  _tasks.push_back(t);
}

void TaskManager::saveCurrentTask(const Task& t)
{
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
  jump(&t);
}
