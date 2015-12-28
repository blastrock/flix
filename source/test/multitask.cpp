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
  printE9("[ALL TESTS RUN]\n");
  sys::call(sys::exit);
}

void loop()
{
  for (unsigned int i = 0; i < 800; ++i)
    xDeb("stuff %d", i++);
  printE9("[END TEST \"multitask1\" OK]\n");
  finish1 = true;
  sys::call(sys::exit);
}

void loop2()
{
  for (unsigned int i = 0; i < 800; ++i)
    xDeb("different stuff %d", i++);
  printE9("[END TEST \"multitask2\" OK]\n");
  finish2 = true;
  sys::call(sys::exit);
}

[[noreturn]] void _main()
{
  Screen::clear();
  Screen::putString("Booting Flix\n");

  auto taskManager = TaskManager::get();

  printE9("[BEGIN TEST \"multitask1\"]\n");
  printE9("[BEGIN TEST \"multitask2\"]\n");

  {
    Task task = taskManager->newKernelTask();
    task.stack = static_cast<char*>(getStackPageHeap().kmalloc().first);
    task.stackTop = task.stack + 0x4000;
    task.kernelStack = static_cast<char*>(getStackPageHeap().kmalloc().first);
    task.kernelStackTop = task.kernelStack + 0x4000;
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
