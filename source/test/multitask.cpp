#include "Debug.hpp"
#include "TaskManager.hpp"
#include "Screen.hpp"
#include "Syscall.hpp"
#include "Symbols.hpp"
#include "Elf.hpp"

XLL_LOG_CATEGORY("main");

volatile bool finish1 = false;
volatile bool finish2 = false;

void waitEnd()
{
  while (!finish1 || !finish2)
    ;
  printE9("\n[END TEST]\n");
  printE9("\n[ALL TESTS RUN]\n");
  sys::call(sys::exit);
}

void loop()
{
  for (unsigned int i = 0; i < 800; ++i)
    xDeb("stuff %d", i++);
  finish1 = true;
  sys::call(sys::exit);
}

void loop2()
{
  for (unsigned int i = 0; i < 800; ++i)
    xDeb("different stuff %d", i++);
  finish2 = true;
  sys::call(sys::exit);
}

[[noreturn]] void _main()
{
  Screen::clear();
  Screen::putString("Booting Flix\n");

  auto taskManager = TaskManager::get();

  printE9("\n[BEGIN TEST \"multitask\"]\n");

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
  {
    Task task = taskManager->newKernelTask();
    task.stack = reinterpret_cast<char*>(0xffffffffa0000000 - 0x4000);
    task.stackTop = task.stack + 0x4000;
    task.kernelStack = static_cast<char*>(getStackPageHeap().kmalloc().first);
    task.kernelStackTop = task.kernelStack + 0x4000;
    task.pageDirectory.mapRange(task.stack, task.stackTop,
        PageDirectory::ATTR_RW);
    task.context.rsp = reinterpret_cast<uint64_t>(task.stackTop);
    task.context.rip = reinterpret_cast<uint64_t>(&loop);
    task.hh.parent = taskManager->getTask(1);
    taskManager->addTask(std::move(task));
  }
  {
    Task task = taskManager->newKernelTask();
    task.stack = reinterpret_cast<char*>(0xffffffffa0000000 - 0x4000);
    task.stackTop = task.stack + 0x4000;
    task.kernelStack = static_cast<char*>(getStackPageHeap().kmalloc().first);
    task.kernelStackTop = task.kernelStack + 0x4000;
    task.pageDirectory.mapRange(task.stack, task.stackTop,
        PageDirectory::ATTR_RW);
    task.context.rsp = reinterpret_cast<uint64_t>(task.stackTop);
    task.context.rip = reinterpret_cast<uint64_t>(&loop2);
    task.hh.parent = taskManager->getTask(1);
    taskManager->addTask(std::move(task));
  }

  xInf("End of kernel");

  taskManager->scheduleNext(); // start a task, never returns

  // this stack will be reused for interrupts when there is no active task

  PANIC("Reached end of main!");
}
