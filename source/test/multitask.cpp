#include "Debug.hpp"
#include "TaskManager.hpp"
#include "Screen.hpp"
#include "Syscall.hpp"
#include "helpers.hpp"

XLL_LOG_CATEGORY("main");

volatile bool childHere = false;

void forkit()
{
  int ret = sys::call(sys::clone, 0, nullptr, nullptr, nullptr, nullptr);
  if (ret < -1)
    th::fail();
  else if (ret == 0)
    childHere = true;
  else
  {
    while (!childHere)
      ;
    sys::call(sys::wait4, ret, nullptr, 0, nullptr);
  }
  sys::call(sys::exit);
}

void testfork()
{
  auto& taskManager = *TaskManager::get();
  pid_t tid;
  {
    Task task = taskManager.newKernelTask();
    task.stack = reinterpret_cast<char*>(0x00000000a0000000 - 0x4000);
    task.stackTop = task.stack + 0x4000;
    task.pageDirectory.mapRange(task.stack, task.stackTop,
        PageDirectory::ATTR_RW | PageDirectory::ATTR_PUBLIC);
    task.kernelStack = static_cast<char*>(getStackPageHeap().kmalloc().first);
    task.kernelStackTop = task.kernelStack + 0x4000;
    task.context.rsp = reinterpret_cast<uint64_t>(task.stackTop);
    task.context.rip = reinterpret_cast<uint64_t>(&forkit);
    tid = taskManager.addTask(std::move(task));
  }
  sys::call(sys::wait4, tid, nullptr, 0, nullptr);
}

void waitEnd()
{
  th::runTestProcesses("double_loop",
      []{
        for (unsigned int i = 0; i < 800; ++i)
          xDeb("stuff %d", i++);
      },
      []{
        for (unsigned int i = 0; i < 800; ++i)
          xDeb("different stuff %d", i++);
      });

  th::runTestProcesses("mmap",
      []{
        sys::call(sys::mmap, nullptr, 0x10000);
      });

  th::runTest("fork", testfork);

  th::finish();
  sys::call(sys::exit);
}

[[noreturn]] void _main()
{
  Screen::getInstance().clear();
  Screen::getInstance().putString("Booting Flix\n");

  auto& taskManager = *TaskManager::get();

  {
    Task task = taskManager.newKernelTask();
    task.stack = static_cast<char*>(getStackPageHeap().kmalloc().first);
    task.stackTop = task.stack + 0x4000;
    task.kernelStack = static_cast<char*>(getStackPageHeap().kmalloc().first);
    task.kernelStackTop = task.kernelStack + 0x4000;
    task.context.rsp = reinterpret_cast<uint64_t>(task.stackTop);
    task.context.rip = reinterpret_cast<uint64_t>(&waitEnd);
    taskManager.addTask(std::move(task));
  }

  xInf("End of kernel");

  taskManager.scheduleNext(); // start a task, never returns

  // this stack will be reused for interrupts when there is no active task

  PANIC("Reached end of main!");
}
