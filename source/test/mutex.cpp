#include "Debug.hpp"
#include "TaskManager.hpp"
#include "Syscall.hpp"
#include "helpers.hpp"

XLL_LOG_CATEGORY("main");

void lockAndUpdate(Mutex& mutex, volatile bool& updating)
{
  for (int i = 0; i < 50; ++i)
  {
    auto lock = mutex.getScoped();
    if (updating)
      th::fail();
    updating = true;
    updating = false;
  }
}

void waitEnd()
{
  Mutex mutex;
  volatile bool updating = false;
  auto lockUpdateFunc = [&]{ lockAndUpdate(mutex, updating); };

  th::runTestProcess("mutex_simple_lock", lockUpdateFunc);

  th::runTestProcesses("mutex_concurrent_lock", lockUpdateFunc, lockUpdateFunc);

  th::finish();
  sys::call(sys::exit);
}

[[noreturn]] void _main()
{
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
