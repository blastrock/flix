#include "Debug.hpp"
#include "TaskManager.hpp"
#include "Screen.hpp"
#include "Syscall.hpp"
#include "Symbols.hpp"
#include "Elf.hpp"

XLL_LOG_CATEGORY("main");

volatile bool finished;

Mutex* mutex;
volatile bool updating = false;

void lockAndUpdate()
{
  for (int i = 0; i < 100; ++i)
  {
    auto lock = mutex->getScoped();
    if (updating)
      printE9("\n[FAIL]\n");
    updating = true;
    updating = false;
  }
  finished = true;
}

void waitEnd()
{
  auto taskManager = TaskManager::get();
  mutex = new Mutex;

  finished = false;
  printE9("\n[BEGIN TEST \"mutex_simple_lock\"]\n");
  {
    Task task = taskManager->newKernelTask();
    task.stack = reinterpret_cast<char*>(0xffffffffa0000000 - 0x4000);
    task.stackTop = task.stack + 0x4000;
    task.kernelStack = static_cast<char*>(getStackPageHeap().kmalloc().first);
    task.kernelStackTop = task.kernelStack + 0x4000;
    task.pageDirectory.mapRange(task.stack, task.stackTop,
        PageDirectory::ATTR_RW);
    task.context.rsp = reinterpret_cast<uint64_t>(task.stackTop);
    task.context.rip = reinterpret_cast<uint64_t>(&lockAndUpdate);
    taskManager->addTask(std::move(task));
  }
  while (!finished)
    ;
  printE9("\n[END TEST]\n");
  printE9("\n[ALL TESTS RUN]\n");
  sys::call(sys::exit);
}

[[noreturn]] void _main()
{
  Screen::clear();
  Screen::putString("Booting Flix\n");

  auto taskManager = TaskManager::get();

  {
    Task task = taskManager->newKernelTask();
    task.stack = reinterpret_cast<char*>(0xffffffffa0000000 - 0x4000);
    task.stackTop = task.stack + 0x4000;
    task.kernelStack = static_cast<char*>(getStackPageHeap().kmalloc().first);
    task.kernelStackTop = task.kernelStack + 0x4000;
    task.pageDirectory.mapRange(task.stack, task.stackTop,
        PageDirectory::ATTR_RW);
    task.context.rsp = reinterpret_cast<uint64_t>(task.stackTop);
    task.context.rip = reinterpret_cast<uint64_t>(&waitEnd);
    taskManager->addTask(std::move(task));
  }

  xInf("End of kernel");

  taskManager->scheduleNext(); // start a task, never returns

  // this stack will be reused for interrupts when there is no active task

  PANIC("Reached end of main!");
}
