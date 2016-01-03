#include "Debug.hpp"
#include "TaskManager.hpp"
#include "Syscall.hpp"
#include "KHeap.hpp"
#include "Timer.hpp"
#include "helpers.hpp"

XLL_LOG_CATEGORY("main");

void mallocDelete(std::size_t s)
{
  auto& heap = KHeap::get();
  char* ptr = static_cast<char*>(heap.kmalloc(s));
  // don't fill everything, it's time consuming
  memset(ptr, 0, std::min(s, 0x100lu));
  heap.kfree(ptr);
}

void simpleMalloc()
{
  mallocDelete(0x0);
  mallocDelete(0x1);
  //mallocDelete(0x10);
  //mallocDelete(0x100);
  //mallocDelete(0x1000);
  mallocDelete(0x10000);
}

void waitEnd()
{
  th::runTest("simple_malloc", simpleMalloc);

  {
    auto loopRun = []{
      for (int i = 0; i < 30; ++i)
        simpleMalloc();
    };
    th::runTestProcesses(
        "concurrent_malloc", loopRun, loopRun);
  }

  th::finish();
  sys::call(sys::exit);
}

[[noreturn]] void _main()
{
  Timer::init(1000);

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
